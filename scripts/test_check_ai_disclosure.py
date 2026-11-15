#!/usr/bin/env python3
"""Unit tests for check_ai_disclosure.py. Run: python3 -m unittest scripts/test_check_ai_disclosure.py"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
from check_ai_disclosure import check  # noqa: E402


def body(level_line: str, reviewed: bool = True) -> str:
    boxes = {
        "None": "- [ ] **None**: I wrote all of this myself.",
        "Assisted": "- [ ] **Assisted**: AI explained or reviewed; I wrote the code.",
        "Generated": "- [ ] **Generated**: AI wrote some or all of it.",
    }
    for level in level_line.split(","):
        if level:
            boxes[level] = boxes[level].replace("[ ]", "[x]")
    review = "[x]" if reviewed else "[ ]"
    return (
        "## Summary\nStuff\n\n## AI assistance\n"
        + "\n".join(boxes.values())
        + f"\n\n## Human review\n- {review} I have read and understand every line of this change.\n"
    )


HUMAN = ("aaa1111", "feat: add thing\n\nBody.")
AI = ("bbb2222", "ci: add workflow\n\nAssisted-by: Claude:claude-opus-5-5")


class CheckAiDisclosure(unittest.TestCase):
    def test_none_with_human_commits_passes(self):
        self.assertEqual(check(body("None"), [HUMAN]).errors, [])

    def test_generated_with_trailer_passes(self):
        self.assertEqual(check(body("Generated"), [HUMAN, AI]).errors, [])

    def test_assisted_without_trailer_passes(self):
        self.assertEqual(check(body("Assisted"), [HUMAN]).errors, [])

    def test_no_level_fails(self):
        self.assertTrue(check(body(""), [HUMAN]).errors)

    def test_two_levels_fail(self):
        self.assertTrue(check(body("None,Generated"), [HUMAN]).errors)

    def test_unticked_review_fails(self):
        self.assertTrue(check(body("None", reviewed=False), [HUMAN]).errors)

    def test_none_contradicted_by_trailer_fails(self):
        errors = check(body("None"), [AI]).errors
        self.assertTrue(any("bbb2222" in e for e in errors))

    def test_generated_without_trailer_fails(self):
        self.assertTrue(check(body("Generated"), [HUMAN]).errors)

    def test_empty_assisted_by_fails(self):
        commit = ("ccc3333", "fix: x\n\nAssisted-by:")
        self.assertTrue(check(body("Generated"), [commit]).errors)

    def test_ai_co_author_fails(self):
        commit = ("ddd4444", "feat: x\n\nCo-authored-by: Claude <noreply@anthropic.com>")
        errors = check(body("Generated"), [commit, AI]).errors
        self.assertTrue(any("Co-authored-by" in e for e in errors))

    def test_human_co_author_passes(self):
        commit = ("eee5555", "feat: x\n\nCo-authored-by: Alex Doe <alex@example.com>")
        self.assertEqual(check(body("None"), [commit]).errors, [])


if __name__ == "__main__":
    unittest.main()
