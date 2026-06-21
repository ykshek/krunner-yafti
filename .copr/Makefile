.PHONY: srpm
srpm:
	# Ensure essential build tools are present in the COPR environment
	dnf install -y git rpm-build gawk

	# Create the standard RPM directory structure
	mkdir -p rpmbuild/{SOURCES,SPECS}

	# Dynamically extract the exact version declared in the spec file
	$(eval VERSION := $(shell grep '^Version:' krunner-yafti.spec | awk '{print $$2}'))

	# Generate the source archive matching the layout expected by %autosetup
	git archive --format=tar.gz --prefix=v$(VERSION)/ HEAD > rpmbuild/SOURCES/v$(VERSION).tar.gz

	# Copy the spec file into the build environment
	cp krunner-yafti.spec rpmbuild/SPECS/

	# Build the SRPM and output it directly to COPR's designated results directory
	rpmbuild --define '_topdir $(PWD)/rpmbuild' -bs rpmbuild/SPECS/krunner-yafti.spec
	cp rpmbuild/SRPMS/*.src.rpm $(outdir)/
