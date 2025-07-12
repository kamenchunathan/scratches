examples_dir := 'examples'
build_dir := 'out'
target_dir := 'target/release/deps'

build:
    just build-platform
    just build-examples
    just link-examples

build-platform:
    #!/usr/bin/env bash
    set -euxo pipefail
    roc check ./platform/libapp.roc
    RUSTFLAGS='--emit=llvm-ir' roc ./scripts/build.roc

build-examples:
    #!/usr/bin/env bash
    set -euo pipefail

    if [ ! -d {{ examples_dir }} ]; then
        echo "Error: Examples directory '{{ examples_dir }}' not found."
        exit 1
    fi

    mkdir -p "{{ build_dir }}/examples/"
    find {{ examples_dir }} -type f -name '*.roc' -print0 | while IFS= read -r -d $'\0' roc_source_file; do
        roc build --linker legacy $roc_source_file --emit-llvm-ir --output  "{{ build_dir }}/examples/" 
    done

run-examples:
    #!/usr/bin/env bash
    set -euo pipefail

    if [ ! -d {{ build_dir }}/examples ]; then
        echo "Error: Build directory '{{ build_dir }}/examples' not found."
        exit 1
    fi

    find {{ build_dir }}/examples -type f -executable -print0 | while IFS= read -r -d $'\0' example; do
        echo "Running $example"
        "$example" || true
    done


link-examples:
    #!/usr/bin/env bash
    set -euo pipefail

    # Check if examples directory exists
    if [ ! -d {{ examples_dir }} ]; then
        echo "Error: Examples directory '{{ examples_dir }}' not found."
        exit 1
    fi

    if [ ! -d "target/release/deps" ]; then
        echo "Error: target/release/deps directory not found."
        exit 1
    fi

    host_file=$(find target/release/deps -name 'roc_host-*.ll' -type f | head -n1)
    if [ -z "$host_file" ]; then
        echo "Error: No roc_host-*.ll file found in target/release/deps"
        exit 1
    fi

    echo "Found host file: $host_file"

    # Patch target in host file
    temp_host_file=$(mktemp)
    trap "rm -f $temp_host_file" EXIT

    sed 's/target triple = "x86_64-unknown-linux-gnu"/target triple = "x86_64-unknown-linux-musl"/' "$host_file" > "$temp_host_file"

    find {{ examples_dir }} -type f -name '*.roc' -print0 | while IFS= read -r -d $'\0' roc_source_file; do
        example_name=$(basename "$roc_source_file" .roc)
        example_ll="examples/${example_name}.ll"
        
        if [ ! -f "$example_ll" ]; then
            echo "Warning: LLVM IR not found for $example_name, skipping..."
            continue
        fi

        echo "Linking $example_name..."
        llvm-link "$temp_host_file" "$example_ll" -o "./out/examples/${example_name}.bc"
        llvm-dis "./out/examples/${example_name}.bc" -o "./out/examples/${example_name}.ll"
        echo "Generated ./out/examples/${example_name}.ll"
    done

    echo "Linking completed successfully!"
    
clean:
    cargo clean
    rm -rf platform/{dynhost,libapp.so,linux-x64.{a,rm},metadata_linux-x64.rm,libapp}
    rm -rf examples/*.ll
    rm -rf "{{ build_dir }}"

