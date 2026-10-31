#!/usr/bin/env python3
"""Valida traducciones y detecta messages nuevos sin entrada en el catálogo inglés."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'Duplicate key: {key!r}')
        result[key] = value
    return result


def load(path):
    return json.loads(path.read_text(encoding='utf-8'), object_pairs_hook=unique_object)


def main():
    parser = argparse.ArgumentParser(description="Validate VETUSTware interface translations.")
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--allow-incomplete', action='store_true')
    args = parser.parse_args()
    root = args.root
    errors = []
    reference = load(root / 'translations/en.json')['messages']
    for key, value in reference.items():
        if key != value:
            errors.append(f'en.json: the value must match the key: {key!r}')
    for path in sorted((root / 'translations').glob('*.json')):
        try:
            doc = load(path)
            if doc.get('language') != path.stem or not re.fullmatch(r'[a-z]{2,3}(?:_[A-Za-z0-9]{2,8})*', path.stem):
                raise ValueError('Invalid language code or filename')
            if not isinstance(doc.get('name'), str) or not doc['name'].strip():
                raise ValueError('Missing native language name')
            messages = doc['messages']
            if not isinstance(messages, dict):
                raise ValueError('messages must be an object')
            for key, value in messages.items():
                if key not in reference:
                    errors.append(f'{path.name}: unknown key: {key!r}')
                if not isinstance(value, str) or not value.strip():
                    errors.append(f'{path.name}: empty or invalid translation: {key!r}')
                    continue
                if Counter(re.findall(r'%L?[1-9][0-9]?', key)) != Counter(re.findall(r'%L?[1-9][0-9]?', value)):
                    errors.append(f'{path.name}: placeholder mismatch: {key!r}')
                if len(key) - len(key.lstrip()) != len(value) - len(value.lstrip()) or len(key) - len(key.rstrip()) != len(value) - len(value.rstrip()):
                    errors.append(f'{path.name}: leading/trailing whitespace mismatch: {key!r}')
            missing = set(reference) - set(messages)
            if missing:
                print(f'{path.name}: {len(missing)} translations missing; English will be used.')
                if not args.allow_incomplete:
                    errors.append(f'{path.name}: incomplete translation')
            print(f'{path.name}: {len(messages)}/{len(reference)} messages')
        except (ValueError, KeyError, TypeError) as error:
            errors.append(f'{path.name}: {error}')
    used = set()
    pattern = re.compile(r'(?:uiText\(|qsTranslate\("Vetust",\s*)("(?:[^"\\]|\\.)*")')
    for path in (root / 'native').iterdir() if (root / 'native').is_dir() else []:
        if path.suffix not in {'.cpp', '.qml'} or path.stem.endswith('_tests'):
            continue
        for match in pattern.finditer(path.read_text()):
            used.add(json.loads(match.group(1)))
    for key in sorted(used - set(reference)):
        errors.append(f'Missing from en.json: {key!r}')
    if errors:
        raise SystemExit('\n'.join(errors))


if __name__ == '__main__':
    main()
