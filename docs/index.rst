
FeatherDoc
==========

Release v\ |version|

*FeatherDoc* is a C++ library for creating and editing Microsoft Word
(.docx) files.


How to install
--------------
The instructions to build and install FeatherDoc

.. code-block:: sh

    cmake -S . -B build
    cmake --build build
    cmake --install build --prefix install

The installed package also carries project-facing metadata and legal files
under ``share/FeatherDoc``.

MSVC Build
----------
Build FeatherDoc with the MSVC toolchain from a Visual Studio Developer
Command Prompt. Use an ``x64`` prompt, or initialize the environment with
``VsDevCmd.bat -arch=x64 -host_arch=x64`` first.

.. code-block:: bat

    cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON
    cmake --build build-msvc-nmake
    ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60

Package Metadata
----------------
The generated CMake package config exposes:

- ``FeatherDoc_VERSION``
- ``FeatherDoc_DESCRIPTION``
- ``FeatherDoc_PACKAGE_DATA_DIR``

Quickstart
--------------
How to start with FeatherDoc quickly

.. code-block:: cpp

    #include <iostream>
    #include <featherdoc.hpp>

    int main() {

        featherdoc::Document doc("file.docx");
        if (const auto error = doc.open()) {
            const auto& error_info = doc.last_error();
            std::cerr << error.message() << std::endl;
            std::cerr << error_info.detail << std::endl;
            return 1;
        }

        for (auto p : doc.paragraphs())
            for (auto r : p.runs())
                std::cout << r.get_text() << std::endl;

        return 0;
    }

Formatting
----------
Use the scoped formatting flags when creating new runs.

.. code-block:: cpp

    paragraph.add_run("bold text", featherdoc::formatting_flag::bold);
    paragraph.add_run(
        "mixed style",
        featherdoc::formatting_flag::bold |
        featherdoc::formatting_flag::italic |
        featherdoc::formatting_flag::underline
    );

Document API
------------
``Document`` now stores its file location as ``std::filesystem::path`` and
exposes explicit success/failure through ``std::error_code`` from ``open()``
and ``save()``. The ``last_error()`` accessor exposes structured failure
context for the most recent operation.

.. code-block:: cpp

    featherdoc::Document doc("file.docx");
    if (const auto error = doc.open()) {
        const auto& error_info = doc.last_error();
        std::cerr << error.message() << std::endl;
        std::cerr << error_info.detail << std::endl;
        return 1;
    }

    if (const auto error = doc.save()) {
        const auto& error_info = doc.last_error();
        std::cerr << error.message() << std::endl;
        std::cerr << error_info.detail << std::endl;
        return 1;
    }

``last_error()`` provides:

- ``code``: the last ``std::error_code``
- ``detail``: richer diagnostic text
- ``entry_name``: the ZIP entry involved in the failure
- ``xml_offset``: parse offset for malformed XML

``save_as(path)`` writes the modified document to a new output path without
changing ``Document::path()``.

.. code-block:: cpp

    if (const auto error = doc.save_as("copy.docx")) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

``create_empty()`` initializes a new in-memory document so callers can produce
fresh ``.docx`` files without opening an existing template archive first.

.. code-block:: cpp

    featherdoc::Document doc("new-file.docx");
    if (const auto error = doc.create_empty()) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

    doc.paragraphs().add_run("Hello FeatherDoc");
    if (const auto error = doc.save()) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

Performance Notes
-----------------
``open()`` now lets ``pugixml`` take ownership of the extracted XML buffer,
which removes one extra copy during parse. ``save()`` now streams both the
updated ``document.xml`` and the unchanged ZIP entries instead of buffering
whole archive entries in memory first.

Bundled Dependencies
--------------------
- ``pugixml`` ``1.15``
- ``kuba--/zip`` ``0.3.8``
- ``doctest`` ``2.5.1``


Contact
--------------
Do you have an issue using FeatherDoc?
Feel free to open an issue in your project repository.

Changelog
---------
Release-facing change history for the current fork:

``CHANGELOG.md``

Project Identity
----------------
Chinese positioning notes for the current FeatherDoc fork:

:doc:`project_identity_zh`

Release Policy
--------------
Chinese release and versioning notes for the current FeatherDoc fork:

:doc:`release_policy_zh`

Project Audit
--------------
Chinese audit and modernization notes for the current fork:

:doc:`project_audit_zh`

License Guide
-------------
Chinese-readable guidance for the current split licensing model:

:doc:`licensing_zh`

Repository-level short references are also available in the project root:
``LEGAL.md`` and ``NOTICE``.

Licensing
--------------
This fork should be described as source-available rather than open source for
its fork-specific modifications.

Fork-specific FeatherDoc modifications are distributed under the
non-commercial source-available terms described in ``LICENSE``.

Portions derived from the upstream DuckX codebase still retain the original
MIT text preserved in ``LICENSE.upstream-mit``. Bundled third-party
dependencies keep their own original licenses.
