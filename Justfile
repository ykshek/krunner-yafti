image_name := "localhost/krunner-yafti:dev"
default: build

build-container:
    #!/usr/bin/env bash
    echo "Building development container image..."
    podman build -t {{image_name}} -f Containerfile .

build: build-container
    #!/usr/bin/env bash
    set -euo pipefail

    echo "Building krunner-yafti..."

    # Create build directory if it doesn't exist
    mkdir -p build

    # Build using development container
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace:Z" \
        --workdir /workspace \
        {{image_name}} \
        bash -c '
            set -euo pipefail
            # Configure and build the project
            cd build
            cmake .. \
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} \
                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX:-/usr}

            make -j$(nproc)
        '

install: build
    #!/usr/bin/env bash
    set -euo pipefail

    # Install using the development container with host system access
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace:Z" \
        --workdir /workspace/build \
        {{image_name}} \
        bash -c '
            # Install to host system
            mkdir -p /workspace/build/prefix
            make install DESTDIR=/workspace/build/prefix
        '
    tree build/prefix
    sudo ostree admin unlock || true
    sudo cp -v ./build/prefix/usr/lib64/qt6/plugins/kf6/krunner/yaftirunner.so /usr/lib64/qt6/plugins/kf6/krunner/yaftirunner.so
    sudo cp -v ./build/prefix/usr/share/locale/es/LC_MESSAGES/plasma_runner_yaftirunner.mo /usr/share/locale/es/LC_MESSAGES/plasma_runner_yaftirunner.mo


gdb:
    gdb -ex "run" -ex "bt" --args ./build/bin/yafti-dbus-tool -s spotify

rpm: build-container
    #!/usr/bin/env bash
    set -euo pipefail
    # Get project version from CMakeLists.txt or use default
    VERSION=$(grep '"Version":' src/metadata.json | sed -E 's/.*"Version": "([^"]+)".*/\1/' || echo "1.0.0")

    # Create RPM build structure
    mkdir -p rpmbuild/{SOURCES,SPECS,BUILD,RPMS,SRPMS}

    # Create source tarball
    echo "Creating source tarball..."
    #git archive --format=tar.gz --prefix=krunner-yafti-${VERSION}/ HEAD > rpmbuild/SOURCES/krunner-yafti-${VERSION}.tar.gz
    tar --exclude='./rpmbuild' --exclude='./build' --exclude='./.git' \
        -czf rpmbuild/SOURCES/v${VERSION}.tar.gz \
        --transform "s|^\.|v${VERSION}|" .

    # Copy spec file
    cp krunner-yafti.spec rpmbuild/SPECS/

    # Build RPM using development container
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace:Z" \
        --workdir /workspace \
        {{image_name}} \
        bash -c "
            set -euo pipefail

            # Build the RPM
            rpmbuild --define '_topdir /workspace/rpmbuild' \
                     --define 'version ${VERSION}' \
                     -ba rpmbuild/SPECS/krunner-yafti.spec
        "

    echo "RPM build complete!"
    echo "Built packages:"
    find rpmbuild/RPMS -name "*.rpm" -type f
    find rpmbuild/SRPMS -name "*.rpm" -type f

bump-version version:
    #!/usr/bin/env bash
    set -euo pipefail

    VERSION="{{version}}"

    # Validate version format (should be like 1.2.3)
    if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        echo "Error: Version must be in format X.Y.Z (e.g., 1.2.3)"
        exit 1
    fi

    echo "Bumping version to $VERSION..."

    # Check if working directory is clean
    if ! git diff --quiet || ! git diff --cached --quiet; then
        echo "Error: Working directory is not clean. Please commit or stash changes first."
        git status --porcelain
        exit 1
    fi

    # Update version in metadata.json
    echo "Updating version in src/metadata.json..."
    sed -i "s/\"Version\": \"[^\"]*\"/\"Version\": \"$VERSION\"/" src/metadata.json

    # Update the version in krunner-yafti.spec
    echo "Updating version in krunner-yafti.spec..."
    sed -i "s/^Version:.*/Version:        $VERSION/" krunner-yafti.spec

    # Verify the change was made in metadata.json
    if ! grep -q "\"Version\": \"$VERSION\"" src/metadata.json; then
        echo "Error: Failed to update version in metadata.json"
        exit 1
    fi

    # Verify the change was made in krunner-yafti.spec
    if ! grep -q "^Version:[[:space:]]*$VERSION" krunner-yafti.spec; then
        echo "Error: Failed to update version in krunner-yafti.spec"
        exit 1
    fi

    echo "Version updated successfully in metadata.json"
    git diff src/metadata.json krunner-yafti.spec
    git add src/metadata.json krunner-yafti.spec
    git commit -m "chore: bump version to v$VERSION"

commit version:
    #!/usr/bin/env bash
    set -euo pipefail

    VERSION="{{version}}"

    # Validate version format (should be like 1.2.3)
    if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        echo "Error: Version must be in format X.Y.Z (e.g., 1.2.3)"
        exit 1
    fi

    echo "Bumping version to $VERSION..."

    # Check if working directory is clean
    if ! git diff --quiet || ! git diff --cached --quiet; then
        echo "Error: Working directory is not clean. Please commit or stash changes first."
        git status --porcelain
        exit 1
    fi

    git push origin HEAD

    git tag "v$VERSION"
    git push origin "v$VERSION"
