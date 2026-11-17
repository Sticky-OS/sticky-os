#!/usr/bin/env python3
"""Checks the board's translations (firmware/sticky-os/src/i18n) and Tutor's (src/tutor):

  - every language file's ids exist, none twice;
  - a translation keeps the English placeholders (%s, %d, %ld...) in order;
  - lists ('|') keep their number of items; plural strings have the forms
    the language needs (one|other, or one|few|many...);
  - every character is one the fonts can draw (ASCII or src/fonts/ExtCodes.h);
  - with --widths: translations much wider than the English (FreeSans 12pt),
    to look at on the screen.

Tutor (src/tutor): t7_strings.h's English rows and each language's pairs (ro.h...): ids, doubles,
placeholders in order, letters; lessons_<code>.h: letters (√ is drawn by the screen). Its page counts
and lengths are checked by firmware/test/tutor7.

  python3 firmware/test/i18n/check.py [--widths] [--missing] [lang ...]
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', 'sticky-os')
I18N = os.path.join(SRC, 'src', 'i18n')
TUTOR = os.path.join(SRC, 'src', 'tutor')
LANGS = ['es', 'pt', 'fr', 'de', 'it', 'ru', 'tr', 'pl', 'ro']
PLURAL_FORMS = {'ru': 3, 'pl': 3, 'ro': 3}  # the rest: 2 (one|other)
LISTS = {'S_DAYS': 7, 'S_DAYS_SHORT': 7, 'S_DAYS_INITIAL': 7, 'S_MONTHS': 12, 'S_MONTHS_SHORT': 12}

def c_string(body):
    """The value of a C string literal body (with escapes), possibly several adjacent literals."""
    out = ''
    for part in re.findall(r'"((?:[^"\\]|\\.)*)"', body):
        out += bytes(part, 'utf-8').decode('unicode_escape').encode('latin-1').decode('utf-8')
    return out

def read_english():
    rows = {}
    order = []
    def load(path):
        for line in open(path, encoding='utf-8'):
            m = re.match(r'\s*S\((S_\w+),\s*(".*")\)', line)
            if m:
                rows[m.group(1)] = c_string(m.group(2))
                order.append(m.group(1))
            inc = re.match(r'#include "(strings_\w+\.h)"', line)
            if inc:
                load(os.path.join(I18N, inc.group(1)))
    load(os.path.join(I18N, 'strings.h'))
    return rows, order

def read_lang(lang):
    pairs, dups = {}, []
    for n, line in enumerate(open(os.path.join(I18N, lang + '.h'), encoding='utf-8'), 1):
        m = re.match(r'\s*\{(S_\w+),\s*(".*")\},', line)
        if not m:
            continue
        if m.group(1) in pairs:
            dups.append(m.group(1))
        pairs[m.group(1)] = c_string(m.group(2))
    return pairs, dups

SPEC = re.compile(r'%[-+ #0]*\d*(?:\.\d+|\.\*)?(?:hh|h|ll|l|z)?[diouxXfFeEgGcsp%]')
def specs(s):
    return [x for x in SPEC.findall(s) if x != '%%']

def strftime_specs(s):
    return sorted(re.findall(r'%[A-Za-z]', s))

def ext_codes():
    text = open(os.path.join(SRC, 'src', 'fonts', 'ExtCodes.h')).read()
    return {int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{4})', text)}

def advances():
    """xAdvance per code point for FreeSans 12pt: ASCII from Adafruit GFX's font, the rest from ours."""
    adv = {}
    ext = open(os.path.join(SRC, 'src', 'fonts', 'FreeSans12ptExt.h')).read()
    for m in re.finditer(r'\{\s*\d+,\s*\d+,\s*\d+,\s*(\d+),\s*-?\d+,\s*-?\d+\s*\}\s*,?\s*// U\+([0-9A-F]{4})', ext):
        adv[int(m.group(2), 16)] = int(m.group(1))
    for base in [os.path.expanduser('~/Documents/Arduino/libraries'), os.path.expanduser('~/Arduino/libraries')]:
        p = os.path.join(base, 'Adafruit_GFX_Library', 'Fonts', 'FreeSans12pt7b.h')
        if os.path.exists(p):
            for m in re.finditer(r'\{\s*\d+,\s*\d+,\s*\d+,\s*(\d+),\s*-?\d+,\s*-?\d+\s*\},?\s*// 0x([0-9A-F]{2})', open(p).read()):
                adv[int(m.group(2), 16)] = int(m.group(1))
            break
    return adv

def width(s, adv):
    return sum(adv.get(ord(c), 13) for c in s)

def check_tutor(codes, err):
    """Tutor's tables: the same rules as the board's strings, by file."""
    en = {}
    for line in open(os.path.join(TUTOR, 't7_strings.h'), encoding='utf-8'):
        m = re.match(r'\s*T7S\((TX_\w+),\s*(".*?")\)\s*(?://.*)?$', line)
        if m:
            if m.group(1) in en:
                err('tutor en', m.group(1), 'twice')
            en[m.group(1)] = c_string(m.group(2))
    def letters(lang, sid, t):
        for c in t:
            if c != '\n' and c != '\u221a' and not (0x20 <= ord(c) < 0x7F) and ord(c) not in codes:
                err('tutor ' + lang, sid, f'"{c}" (U+{ord(c):04X}) is not in the fonts')
    for sid, t in en.items():
        letters('en', sid, t)
    for name in sorted(os.listdir(TUTOR)):
        m = re.match(r'([a-z]{2})\.h$', name)
        if m:
            lang, seen = m.group(1), set()
            for line in open(os.path.join(TUTOR, name), encoding='utf-8'):
                p = re.match(r'\s*\{(TX_\w+),\s*(".*")\},', line)
                if not p:
                    continue
                sid, t = p.group(1), c_string(p.group(2))
                if sid in seen:
                    err('tutor ' + lang, sid, 'twice')
                seen.add(sid)
                if sid not in en:
                    err('tutor ' + lang, sid, 'no such id')
                    continue
                letters(lang, sid, t)
                if specs(en[sid]) != specs(t):
                    err('tutor ' + lang, sid, f'placeholders {specs(t)}, English has {specs(en[sid])}')
            missing = [s for s in en if s not in seen]
            if missing:
                print(f'tutor {lang}: {len(missing)} of {len(en)} not translated: Tutor shows English for all its content in {lang}')
        m = re.match(r'lessons_([a-z]{2})\.h$', name)
        if m:
            for n, line in enumerate(open(os.path.join(TUTOR, name), encoding='utf-8'), 1):
                if line.lstrip().startswith('//'):
                    continue
                for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', line):
                    letters('lessons ' + m.group(1), f'line {n}', c_string('"' + lit + '"'))
    return len(en)

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    show_widths, show_missing = '--widths' in sys.argv, '--missing' in sys.argv
    en, order = read_english()
    codes = ext_codes()
    adv = advances() if show_widths else {}
    errors = 0
    def err(lang, sid, msg):
        nonlocal errors
        errors += 1
        print(f'{lang} {sid}: {msg}')
    for sid, s in en.items():
        for c in s:
            if not (0x20 <= ord(c) < 0x7F) and ord(c) not in codes:
                err('en', sid, f'U+{ord(c):04X} is not in the fonts')
    for lang in args or LANGS:
        pairs, dups = read_lang(lang)
        for d in dups:
            err(lang, d, 'twice')
        for sid, t in pairs.items():
            if sid not in en:
                err(lang, sid, 'no such id')
                continue
            e = en[sid]
            for c in t:
                if not (0x20 <= ord(c) < 0x7F) and ord(c) not in codes:
                    err(lang, sid, f'"{c}" (U+{ord(c):04X}) is not in the fonts')
            if sid in LISTS:
                if len(t.split('|')) != LISTS[sid]:
                    err(lang, sid, f'{len(t.split("|"))} items, not {LISTS[sid]}')
                continue
            if '|' in e and '%ld' in e:  # plural forms
                need = PLURAL_FORMS.get(lang, 2)
                forms = t.split('|')
                if len(forms) != need:
                    err(lang, sid, f'{len(forms)} plural forms, {lang} needs {need}')
                for f in forms:
                    if specs(f) and specs(f) != ['%ld']:
                        err(lang, sid, f'a plural form with {specs(f)}: only %ld')
                continue
            if e.count('|') != t.count('|'):
                err(lang, sid, f"{e.count('|')} '|' in English, {t.count('|')} here")
            if specs(e) != specs(t):
                err(lang, sid, f'placeholders {specs(t)}, English has {specs(e)}')
            if strftime_specs(e) != strftime_specs(t) and sid.startswith(('S_FMT', 'S_CAL_', 'S_UPDATED', 'S_NOT_UPDATED', 'S_TIMER', 'S_CLAUDE_RESETS', 'S_NOTE_ADDED', 'S_SAVED_ON', 'S_HOURS_TODAY', 'S_QUOTE_OF_DAY_DATE', 'S_TELEMETRY_SENT_AT')):
                if any(x in e for x in ('%H', '%M', '%a', '%A', '%b', '%B', '%e')):
                    err(lang, sid, f'date fields {strftime_specs(t)}, English has {strftime_specs(e)}')
            if show_widths:
                we, wt = width(e, adv), width(t, adv)
                if we and wt > we * 1.25 and wt - we > 30:
                    print(f'  width {lang} {sid}: {wt} px vs {we} px English: "{t}"')
        missing = [s for s in order if s not in pairs]
        if missing:
            print(f'{lang}: {len(missing)} of {len(order)} not translated (English shows)' + (': ' + ' '.join(missing) if show_missing else ''))
    tutor = check_tutor(codes, err)
    print(f'{len(en)} strings, {tutor} Tutor strings, {errors} problems')
    return 1 if errors else 0

if __name__ == '__main__':
    sys.exit(main())
