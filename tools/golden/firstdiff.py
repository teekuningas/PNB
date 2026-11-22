# firstdiff.py A B — validate both traces, then report their first difference.
import itertools
import re
import sys

NAMES = ["players", "ball", "possession", "play", "flow", "pitch",
         "throw", "swing", "rules", "client", "ai", "out"]
FRAME = re.compile(r"([0-9]+)(?: [0-9a-f]{16}){12}\n")
END = re.compile(r"# end frame=([0-9]+) failed=0 reason=-\n")
FOOTERS = ("# end ", "# coverage ", "# walks ", "# flow ")


def trace_lines(path):
    last_frame = None
    footer = 0
    with open(path) as stream:
        for number, line in enumerate(stream, 1):
            if line.startswith("#"):
                if footer >= len(FOOTERS) or not line.startswith(FOOTERS[footer]) or not line.endswith("\n"):
                    raise ValueError(f"{path}:{number}: malformed or unexpected footer")
                if footer == 0:
                    end = END.fullmatch(line)
                    if not end or last_frame is None or int(end[1]) != last_frame:
                        raise ValueError(f"{path}:{number}: failed run or invalid end frame")
                footer += 1
            else:
                frame = FRAME.fullmatch(line)
                if footer or not frame or (last_frame is not None and int(frame[1]) != last_frame + 1):
                    raise ValueError(f"{path}:{number}: malformed or nonconsecutive frame")
                last_frame = int(frame[1])
            yield line
    if last_frame is None or footer != len(FOOTERS):
        raise ValueError(f"{path}: empty or incomplete trace")


def compare(a, b):
    first = None
    for left, right in itertools.zip_longest(trace_lines(a), trace_lines(b)):
        if left == right or first is not None:
            continue
        if left is None or right is None:
            first = "different lengths"
        elif left.startswith("#") or right.startswith("#"):
            first = f"meta: {left.strip()} | {right.strip()}"
        else:
            fa, fb = left.split(), right.split()
            if fa[0] != fb[0]:
                first = f"frame numbers: {fa[0]} | {fb[0]}"
            else:
                sections = [name for name, x, y in zip(NAMES, fa[1:], fb[1:]) if x != y]
                first = f"frame {fa[0]} sections: {sections}"
    if first is not None:
        print(first)
        return 1
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: firstdiff.py A B", file=sys.stderr)
        sys.exit(2)
    try:
        sys.exit(compare(sys.argv[1], sys.argv[2]))
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        sys.exit(2)
