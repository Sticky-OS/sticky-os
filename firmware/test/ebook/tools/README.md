# Tools for the book code

- `make_fixtures.py`: makes the small test books in `../fixtures` (they are
  committed; run it again only to change one): `python3 tools/make_fixtures.py`.
- `book2flow.cpp`: converts any book with the firmware's converter and prints
  its meta, the numbers (sizes, steps, longest step, time), the contents and,
  with `-d`, the flow as readable text (`-n 20000`: only its first bytes).
  `book2flow.sh` builds it with run.sh's flags and objects and runs it:

      tools/book2flow.sh -n 20000 ~/books/alice.epub /tmp/out | less

  It writes `<outdir>/<book's name>.flow`, `.flow.toc` and `.cover`.
