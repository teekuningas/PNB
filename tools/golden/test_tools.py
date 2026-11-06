import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

TOOLS = Path(__file__).resolve().parent
HARNESS = Path(sys.argv.pop(1)).resolve()
FAILURE_HARNESS = Path(sys.argv.pop(1)).resolve()


def trace(hash_value="0000000000000000", failed=0):
    return (
        "1 " + " ".join([hash_value] * 12) + "\n"
        f"# end frame=1 failed={failed} reason={'probe' if failed else '-'}\n"
        "# coverage frames=1 windups=0 pitches=0 fakes=0 throws=0 hits=0\n"
        "# walks offers=0 midWindup=0\n"
        "# flow halfInnings=0 runs=0 honors=0 fouls=0 pairEnds=0 foulDuringPairEnd=0\n"
    )


class GoldenToolsTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="pnb-golden-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.left = self.root / "before with spaces"
        self.right = self.root / "after with spaces"
        self.left.mkdir()
        self.right.mkdir()

    def run_tool(self, tool, *args, env=None):
        return subprocess.run(
            [str(tool), *map(str, args)], cwd=TOOLS.parents[1],
            env=env, text=True, capture_output=True, timeout=60,
        )

    def compare(self, left=None, right=None):
        return self.run_tool(TOOLS / "compare.sh", left or self.left, right or self.right)

    def pair(self, left=None, right=None):
        (self.left / "sim0.trace").write_text(trace() if left is None else left)
        (self.right / "sim0.trace").write_text(trace() if right is None else right)

    def test_identical_complete_traces_pass(self):
        self.pair()
        result = self.compare()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("identical: 1 / 1", result.stdout)

    def test_difference_reports_first_frame_and_fails(self):
        self.pair(right=trace("1111111111111111"))
        result = self.compare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("frame 1 sections: ['players'", result.stdout)
        self.assertIn("identical: 0 / 1", result.stdout)

    def test_missing_runs_fail_in_both_directions(self):
        self.pair()
        (self.left / "left-only.trace").write_text(trace())
        (self.right / "right-only.trace").write_text(trace())
        result = self.compare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("left-only: only in", result.stdout)
        self.assertIn("right-only: only in", result.stdout)

    def test_empty_or_missing_recordings_fail(self):
        for right in (self.right, self.root / "missing"):
            with self.subTest(right=right):
                self.assertNotEqual(self.compare(right=right).returncode, 0)

    def test_identical_bad_traces_are_not_a_baseline(self):
        for bad in ("", trace(failed=1), trace().split("# end")[0],
                    trace().rsplit("# flow", 1)[0], trace().rstrip("\n"),
                    trace().replace("1 0000000000000000", "1 bad", 1)):
            with self.subTest(bad=bad):
                self.pair(bad, bad)
                result = self.compare()
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("Traceback", result.stderr)

    def test_footer_difference_fails(self):
        self.pair(right=trace().replace("hits=0", "hits=1"))
        result = self.compare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("meta:", result.stdout)

    def fake_environment(self, **settings):
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        runner = bin_dir / "runner"
        runner.write_text(
            f"#!{sys.executable}\n"
            "import os, pathlib, sys, time\n"
            "out, run = pathlib.Path(sys.argv[1]), sys.argv[2]\n"
            "if os.environ.get('FAKE_WAIT'):\n"
            "    (out / (run + '.started')).touch()\n"
            "    time.sleep(60)\n"
            "if run == os.environ.get('FAKE_NO_TRACE_RUN'): sys.exit(0)\n"
            f"content = {trace()!r}\n"
            "if run == os.environ.get('FAKE_FAILED_FOOTER'): content = content.replace('failed=0', 'failed=1')\n"
            "(out / (run + '.trace')).write_text(content)\n"
            "if run == os.environ.get('FAKE_FAIL_RUN'): sys.exit(7)\n"
        )
        runner.chmod(0o755)
        nix = bin_dir / "nix"
        nix.write_text(
            f"#!{sys.executable}\n"
            "import os, pathlib, shutil, sys\n"
            "args = sys.argv[sys.argv.index('--command') + 1:]\n"
            "if args[0] == 'gcc':\n"
            f"    shutil.copyfile({str(runner)!r}, args[args.index('-o') + 1])\n"
            "    pathlib.Path(args[args.index('-o') + 1]).chmod(0o755)\n"
            "elif args[-1] == 'scripted_runner' and os.environ.get('FAKE_BUILD_FAIL'):\n"
            "    print('injected build error', file=sys.stderr)\n"
            "    sys.exit(9)\n"
        )
        nix.chmod(0o755)
        return {**os.environ, "PATH": f"{bin_dir}:{os.environ['PATH']}", **settings}

    def test_record_waits_for_all_runs_and_cleans_worktree_on_failure(self):
        env = self.fake_environment(FAKE_FAIL_RUN="sim0")
        out = self.root / "record"
        result = self.run_tool(TOOLS / "record.sh", out, "HEAD", env=env)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("sim0 failed", result.stderr)
        self.assertNotIn("recorded 66", result.stdout)
        self.assertEqual(len(list(out.glob("*.trace"))), 66)
        self.assertFalse((out / ".tree").exists())

    def test_record_rejects_missing_or_failed_trace_even_if_child_exits_zero(self):
        for setting in ("FAKE_NO_TRACE_RUN", "FAKE_FAILED_FOOTER"):
            with self.subTest(setting=setting):
                # Each case owns an isolated tool environment and output directory.
                env = self.fake_environment(**{setting: "sim0"})
                result = self.run_tool(TOOLS / "record.sh", self.root / setting, env=env)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("sim0 has no successful trace", result.stderr)
                (self.root / "bin" / "nix").unlink()
                (self.root / "bin" / "runner").unlink()
                (self.root / "bin").rmdir()

    def test_record_build_error_is_visible_and_worktree_is_removed(self):
        env = self.fake_environment(FAKE_BUILD_FAIL="1")
        out = self.root / "record"
        result = self.run_tool(TOOLS / "record.sh", out, "HEAD", env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("injected build error", result.stderr)
        self.assertFalse((out / ".tree").exists())

    def test_record_interruption_stops_children_and_removes_worktree(self):
        env = self.fake_environment(FAKE_WAIT="1")
        out = self.root / "record"
        process = subprocess.Popen(
            [str(TOOLS / "record.sh"), str(out), "HEAD"], cwd=TOOLS.parents[1],
            env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )
        try:
            deadline = time.monotonic() + 15
            while not (out / "sim0.started").exists() and process.poll() is None and time.monotonic() < deadline:
                time.sleep(0.05)
            self.assertTrue((out / "sim0.started").exists(), "the recorder never launched a child")
            process.terminate()
            stdout, stderr = process.communicate(timeout=15)
            self.assertEqual(process.returncode, 143, stderr)
            self.assertFalse((out / ".tree").exists())
            self.assertNotIn("recorded 66", stdout)
        finally:
            if process.poll() is None:
                process.terminate()
                process.communicate(timeout=15)

    def test_record_success_and_stale_directory_rejection(self):
        env = self.fake_environment()
        repo_link = self.root / "repo link"
        repo_link.symlink_to(TOOLS.parents[1], target_is_directory=True)
        with tempfile.TemporaryDirectory(prefix=".golden-test-", dir=TOOLS.parents[1]) as inside:
            result = self.run_tool(repo_link / "tools/golden/record.sh", inside, env=env)
            self.assertEqual(result.returncode, 2, result.stderr)
            self.assertIn("OUTDIR must be outside the repository", result.stderr)
            self.assertEqual(list(Path(inside).iterdir()), [])
        out = self.root / "record"
        result = self.run_tool(TOOLS / "record.sh", out, env=env)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("recorded 66", result.stdout)
        result = self.run_tool(TOOLS / "record.sh", out, env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OUTDIR must be empty", result.stderr)
        self.assertEqual(len(list(out.glob("*.trace"))), 66)

    def test_real_golden_loops_propagate_sim_and_scripted_failures(self):
        for run in ("sim0", "h2h0"):
            with self.subTest(run=run):
                result = self.run_tool(FAILURE_HARNESS, self.root, run)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn("failed=1 reason=injected", (self.root / f"{run}.trace").read_text())

    def test_trace_write_errors_are_not_success(self):
        target = self.root / "sim0.trace"
        target.symlink_to("/dev/full")
        result = self.run_tool(HARNESS, self.root, "sim0")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("could not write complete trace", result.stderr)


if __name__ == "__main__":
    unittest.main()
