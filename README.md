# KRunner Yafti Plugin

This is basically 50% copied from [Krunner-Bazaar](https://github.com/bazaar-org/krunner-bazaar/) and 50% vibe coded.
Either someone comes along and take a good look at it or I try to comprehend what is happening bit by bit before this goes on anything.

I am also not sure about the ownership about the files(i.e. whether they should be left with their original owner, re-author with my name, and/or add co-authored by LLM). For now I've edited them to be under my name, but I am happy to change whatever needs to be changed.

## Installation

From Source

Requires Podman. Tested on Bazzite.
```bash
just build
just install
```
Building RPM Locally
```bash
# Build RPM package
just rpm
```
## Development

### Internationalization

Haven't gotten around to it yet.

### Release Process

Haven't gotten around to it yet.

## Implementation Details

It takes a look at `/usr/share/yafti/yafti.yml`, matches it, then uses `which` to find the location of `yafti_gtk.py`(or fallback to `/usr/bin/yafti_gtk.py`?), then if the entry is selected in krunner, run the script with the `--action-id ID` argument, with ID being the id entry of the `yafti.yml` file. This means this is also reliant on [this](https://github.com/ublue-os/yafti-gtk/pull/19)

...well, from what I understand, anyways.
