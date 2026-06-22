# KRunner Yafti Plugin

A krunner plugin that looks at the YAML file for Bazzite Portal, puts it in Plasma Search, then calls Bazzite Portal to highlight the option if it is selected.

## Code Origins

This is only made possible by:

- Heavy inspiration from the [Krunner-Bazaar](https://github.com/bazaar-org/krunner-bazaar/) project,
- Some very helpful documentation on [develop.kde.org](https://develop.kde.org/docs/plasma/krunner/),
- Some liberal use of LLMs (not agentic AI!) to help set up stuff like the header and build initially.

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
note: It will try to build against the latest tag, you'll need to make a new release/tag first to build the rpm.

## Development

### Internationalization

Will be added once Bazzite Portal has that functionality.

### Release Process

Create a version tag:
```bash
just bump-version 1.x.x
```
Verify your changes, then run the following to add the appropriate tag and push the change:
```bash
just commit 1.x.x
```
GitHub Actions will hopefully automatically:

- Build the RPM packages
- Create a GitHub release
- Upload RPM artifacts with checksums

## Implementation Details

The `loadActions` function takes a look at `/usr/share/yafti/yafti.yml`, matches it, then runs `/usr/bin/yafti_gtk.py` (or use `which` to find the location of `yafti_gtk.py` as a fallback). At this point the yml will be loaded into memory, and anything with the `hidden` key will be ignored.

The `match` function is run by krunner/Plasma Search. The `relevance` variable is set here, and as krunner displays results in order of relevance, we do some manipulation here to make sure
1. results are displayed in order of the yaml;
2. title matches are displayed in front of description matches.

The `run` function runs when the entry is selected in krunner. the script is run with the `--action-id ID` argument, with ID being the id entry of the `yafti.yml` file. A little proprocessing is done here because `match.Id` obtained previously has a `krunner-yafti_` prefix, which is not needed. 

This means this is also reliant on [this](https://github.com/ublue-os/yafti-gtk/pull/19), which adds the highlighting part when an action ID is specified in the command arguments.
