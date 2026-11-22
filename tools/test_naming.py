from pathlib import Path
import subprocess
import tempfile
import unittest

MEASURE = Path(__file__).resolve().with_name("naming_debt.sh")


class NamingMeasurementTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="pnb naming ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for directory in ("src", "tests", "tools"):
            (self.root / directory).mkdir()

    def measure(self):
        return subprocess.run(
            ["bash", str(MEASURE), "-Isrc", str(self.root)],
            cwd=self.root, text=True, capture_output=True, timeout=30,
        )

    def test_fields_not_type_names_comments_globals_or_function_locals(self):
        (self.root / "src" / "types.h").write_text(
            "#define DECLARE_FIELD int macroField\n"
            "typedef struct CamelOwner {\n"
            "    int plain; int camelField; int *arrayField[2]; DECLARE_FIELD;\n"
            "} CamelOwner;\n"
            "// int commentField;\n"
            "extern int globalField;\n"
            "enum CamelEnum { camelValue };\n"
        )
        (self.root / "src" / "body.c").write_text(
            '#include "types.h"\n'
            "typedef struct { int privateField; } Private;\n"
            "void work(void) { struct { int localField; } local; }\n"
        )
        result = self.measure()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "4\t0")

    def test_abbreviations_are_words_across_all_code_not_other_files(self):
        (self.root / "src" / "types.h").write_text(
            "typedef struct { int plain; } Plain;\n"
            "// tPI cPI x_tPI bTPI_suffix\n"
        )
        (self.root / "tests" / "test.c").write_text('// cTPI bTPI\n')
        (self.root / "tools" / "probe.h").write_text('// tPI\n')
        (self.root / "tools" / "ignored.py").write_text("# tPI\n")
        result = self.measure()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "0\t5")

    def test_parse_errors_are_not_a_clean_count(self):
        (self.root / "src" / "broken.h").write_text(
            "typedef struct { int brokenField ???; } Broken;\n"
        )
        result = self.measure()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("error:", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_missing_code_is_not_a_clean_count(self):
        result = self.measure()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no C sources", result.stderr)


if __name__ == "__main__":
    unittest.main()
