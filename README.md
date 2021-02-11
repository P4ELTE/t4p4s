
# T₄P₄S, a multitarget P4<sub>16</sub> compiler

This is an experimental compiler for P4<sub>16</sub> and P4<sub>14</sub> files.
For publications and more, [see our homepage](http://p4.elte.hu/).

An older version of the compiler is [also available](https://github.com/P4ELTE/t4p4s/tree/t4p4s-14).

Find out more [about the P4 language](https://p4.org/).

## Getting started

### Preparation

To start working with the compiler, simply download the `bootstrap-t4p4s.sh` script and execute it in the following way.
The script installs all necessary libraries ([DPDK](https://www.dpdk.org/), [P4C](https://github.com/p4lang/p4c), `P4Runtime` and more) and T₄P₄S itself, and sets up environment variables.

    wget https://raw.githubusercontent.com/P4ELTE/t4p4s/master/bootstrap-t4p4s.sh
    chmod +x bootstrap-t4p4s.sh
    . ./bootstrap-t4p4s.sh

Notes.

- ⚠ The purpose of the script is to setup a convenient environment for you if you're just starting out.
  Therefore, you *only need to execute it once*.
  If you already have a working environment, you *don't need to run the script again*, and you *probably shouldn't*.

    - To get T₄P₄S only without the third party libraries: `git clone --recursive https://github.com/P4ELTE/t4p4s`
    - To update a previous T₄P₄S checkout, execute this command in its directory: `git pull --recurse-submodules`

- Without the `.` at the beginning of the line, the environment variables will not be usable immediately.
    - In that case, you can either open a new terminal, or run `. ./t4p4s_environment_variables.sh`

- The script is intended to work on recent Debian based systems, e.g. the latest LTS edition of Linux Mint or Ubuntu.
    - Legacy systems such as Ubuntu 18.04 and 16.04 are not supported by the script, as they do not come with out-of-the-box support for sufficiently recent libraries (such as Meson 0.47.1 or newer required for building DPDK, or Python 3.8 or newer required for building T₄P₄S).
    - Even on legacy systems, the script may still be useful to you. You may disable stages of the script, and manually install the software.

    INSTALL_STAGE2_DPDK=no INSTALL_STAGE3_PROTOBUF=no INSTALL_STAGE4_P4C=no . ./bootstrap-t4p4s.sh

    - It is also useful if you are not interested in using `P4Runtime` features.

    INSTALL_STAGE5_P4RT=no . ./bootstrap-t4p4s.sh

- To see all possible options (including available stages), run the script the following way.

    ./bootstrap-t4p4s.sh showenvs

- If you happen to have some of the dependencies locally checked out, you can speed up the installation process by letting the script clone them locally.

    LOCAL_REPO_CACHE=/my/cache/dir . ./bootstrap-t4p4s.sh

- At this stage of development, T₄P₄S will not compile and run all P4 programs properly. In particular, header stacks are not supported currently.


Overriding defaults.

- To increase efficiency, the script runs jobs on all cores of the system in parallel. Should you experience any problems (for example, your system may run out of memory), you can override the number of jobs.

    MAX_MAKE_JOBS=4 . ./bootstrap-t4p4s.sh

- By default, the script runs downloads in parallel. You can force it to work sequentially.

    PARALLEL_INSTALL=no . ./bootstrap-t4p4s.sh

- The script installs the newest versions of DPDK and P4C unless overridden by the user.

    DPDK_VERSION=20.05 . ./bootstrap-t4p4s.sh
    DPDK_VERSION=20.05 DPDK_FILEVSN=20.05.0 . ./bootstrap-t4p4s.sh
    P4C_COMMIT_DATE=20201101 . ./bootstrap-t4p4s.sh

- The script uses `clang`, `clang++` and `lld` by default if they are installed unless overridden. It also uses `ccache`.

    T4P4S_CC=gcc T4P4S_CXX=g++ T4P4S_LD=bfd . ./bootstrap-t4p4s.sh


### Options

In the `t4p4s.sh` script, options control the process of compilation and execution.
The options are collected in the following phases.

1. By default, the `colours.cfg`, `lights.cfg`, the command line, `examples.cfg` and `opts_${ARCH}.cfg` are processed.
    - `colours.cfg` describes the available colours for output highlighting.
    - `lights.cfg` describes which colours are used in the terminal and in the switch output for highlighting.
    - `examples.cfg` sets options for each example.
    - `opts_${ARCH}.cfg` sets architecture specific options.
    - Currently, the only valid value for `${ARCH}` is `dpdk`.
1. When the command line of the script is processed, anything not identifiable as a P4 program is considered an option.
    - A P4 program is the name of an existing file whose extension begins with `p4`.
    - Here, the options are separated by spaces, therefore their values are not allowed to contain spaces themselves.
1. Option files come in two flavours.
    - Some files (e.g. `lights.cfg`) contain an option definition on a single line.
    - Some files (e.g. `examples.cfg`) contain an example identifier and then any number of options on a line.
        - An example identifier is `examplename@testcasename`, or if `@testcasename` is not given, `@std` is used by default.
        - As in the case of the command line, options may not contain any spaces.
    - In both cases, empty lines (containing whitespace only) and comments (a `;` not preceded by a number, until the end of the line) are ignored.

The format of option definitions is the following.

- Option names contain letters, numbers, `-` (dash), `_` (underscore) and `.` (dot).
- Define `myopt` with the value `myval`.
    `myopt=myval`
- Define `myopt` that takes the default value `on`.
    `myopt`
- In the cases described above, spaces can be allowed.
    `myopt=foo bar`
- From this point on, `myopt` is ignored: it is considered not to be defined.
    `^myopt`
- Define `myopt` with the value `val` only if `mycondopt` _is defined_ at this time.
    `mycondopt->myopt=val`
- Define `myopt` with the value `val` only if `mycondopt` is _not_ defined at this time.
    `^mycondopt->myopt=val`
- Define `myopt` with the value `val` only if `mycondopt` is defined at this time, and its value is `condval`.
    `mycondopt=condval->myopt=val`
- In all of the above, `+=` `++=` can take the place of `=`. Instead of setting the option, they append to the current value: `+=` with a space separator, `++=` with a newline.
  - `myopt=foo` and then `myopt+=bar` is equivalent to `myopt=foo bar`
- For convenience, there are some abbreviations.

    | Option given            | Equivalent to                                                           |
    | ----------------------- | ----------------------------------------------------------------------- |
    | @myvariant              | variant=**myvariant**                                                   |
    | :myexample              | example=**myexample**                                                   |
    | ::myexample             | example=**myexample** dbg                                               |
    | %myexample=mytestcase   | example=**myexample** variant=test testcase=**mytestcase**              |
    | %myexample              | example=**myexample** variant=test testcase=test                        |
    | %%myexample=mytestcase  | example=**myexample** variant=test verbose dbg testcase=**mytestcase**  |
    | %%myexample             | example=**myexample** variant=test verbose=lines dbg suite                    |

### Crypto devices for cryptography operations

It is hardcoded in the current prototype to create an OpenSSL-based virtual crypto device in DPDK in order to support encryption and decryption extern functions. The PMD for this virtual device is not compiled in DPDK by default.

To enable the OpenSSL crypto PMD, edit **dpdk-19.02/config/common_base** by changing

~~~
CONFIG_RTE_LIBRTE_PMD_OPENSSL=n
~~~

to

~~~
CONFIG_RTE_LIBRTE_PMD_OPENSSL=y
~~~

and do a rebuild on DPDK.

### Execution

The `t4p4s.sh` script uses settings from three configuration files.

1. `light.cfg` describes how texts in the terminal and switch output look.
1. `examples.cfg` describes default options for the examples.
    - A set of parameters for an example is called a configuration _variant_.
    - On the command line, you have to specify the _example_ (by name or full path) and the _variant name_.
1. An architecture specific file (for DPDK, `opts_dpdk.cfg`) describes how the options are to be interpreted: they are translated to more options.
    - Everything apart from the _example_ is considered an option on the command line.

The script returns an exit code of `0` if the execution was successful, and a non-zero value otherwise.

The script creates `build/<example-name>`.

- Under it, the directories `build`, `srcgen` and `Makefile` contain compilation artifacts, including the created switch executable.
- Log output is stored in `log`.
    - `controller.log` is the log output from the most recent controller execution.
    - For each execution, two log files are created.
        - The one with the simple `.txt` extension is a regular textual log.
        - The one with the `lit.txt` extension contains ANSI colour codes. Invoking `cat` on it, or using an appropriate viewer like [SublimeANSI](https://github.com/aziz/SublimeANSI) will show coloured output.
        - The logs of the most recent script execution are also available as `last.txt` and `last.lit.txt`.


### Examples

Note that for non-testing examples, you will have to setup your network card, and probably adjust your configuration options.

1. Specify an example
    - Run an example with the default configuration
        `./t4p4s.sh :l2fwd`
    - The program finds the source file under `examples` automatically, but you can also specify it manually
        `./t4p4s.sh ./examples/l2fwd.p4_14`
1. Execution phases, option settings
    - Specify one or more steps to be taken
        `./t4p4s.sh :l2fwd p4`
        `./t4p4s.sh :l2fwd c`
        `./t4p4s.sh :l2fwd run`
    - If no option is given, all phases (`p4 c run`) are active
        `./t4p4s.sh :l2fwd`
    - Options can be given in any order (phases will always run in `p4 c run` order)
        `./t4p4s.sh :l2fwd p4 c`
        `./t4p4s.sh :l2fwd c p4`
    - All options have one parameter, which defaults to "on"
        `./t4p4s.sh :l2fwd p4=on c=on run=on`
    - Prefixing an option with `^` suppresses it
        - Run only P4-to-C and C-to-switch compilation
        `./t4p4s.sh :l2fwd ^run`
    - Set the controller configuration (the controller program takes it as a parameter)
        `./t4p4s.sh :l2fwd ctrcfg=my_ctr_opts.txt`
1. Output options: highlighting, verbosity
    - Get monochrome (black-and-white) output, useful for scripting
        `./t4p4s.sh :l2fwd bw`
    - Monochrome terminal, colour switch execution
        `./t4p4s.sh :l2fwd bw=terminal`
    - Colour terminal, monochrome switch execution
        `./t4p4s.sh :l2fwd bw=switch`
    - Verbose output for the terminal
        `./t4p4s.sh :l2fwd verbose`
    - Verbose output for the switch
        `./t4p4s.sh :l2fwd dbg`
    - In addition, statistics can be displayed at the end
        `./t4p4s.sh :l2fwd dbg stats`
    - Suppress EAL messages from the switch output
        `./t4p4s.sh :l2fwd noeal`
    - No output at all (both terminal and switch) except for errors
        `./t4p4s.sh :l2fwd silent`
    - If the switch fails, runs it again in the debugger (by default, `gdb`)
        `./t4p4s.sh :l2fwd autodbg`
1. Variants, testing
    - Specify a variant, a set of configuration options
        `./t4p4s.sh :l2fwd @test`
        `./t4p4s.sh :l2fwd variant=test`
    - Run a single test case
        - It runs offline: no network card is needed
        - Data for the test case is in `examples/test-l2fwd.c`
        `./t4p4s.sh :l2fwd @test testcase=test`
        `./t4p4s.sh :l2fwd @test testcase=payload`
    - Abbreviated form (also sets `@test`)
        `./t4p4s.sh %l2fwd=payload`
    - Another abbreviation, equivalent to using `testcase`, `dbg` and `@test`
        `./t4p4s.sh ::l2fwd`
    - Run the test suite for the example
        `./t4p4s.sh %%l2fwd`
    - Stop the switch immediately upon encountering invalid data
        `./t4p4s.sh %l2fwd=payload strict`
1. Redo
    - `t4p4s.sh` saves the collected environment variables to `build/l2fwd-gen@test-test/redo.opts.txt` (when executed as `./t4p4s.sh %l2fwd`)
    - This option loads the saved environment; can speed up rerunning test cases
        - Mostly useful for development purposes
    - Has to be the very first argument to `t4p4s.sh`
    - `run_tests.sh` (see below) also uses this option
        `./t4p4s.sh redo=%l2fwd`
        `./t4p4s.sh redo=%l2fwd=test2`
1. Hugepages
    - `examples.cfg` sets the required number of hugepages for each example
    - Set it to another value, e.g. make T₄P₄S use `1024 MB` of hugepages
        `./t4p4s.sh %l2fwd hugemb=1024`
    - You may specify the amount of hugepages instead of the desired size in megabytes, which is dependent on the size of the hugepages on your system
        `./t4p4s.sh %l2fwd hugepages=1024`
    - Instruct `t4p4s.sh` not to modify the current number of hugepages (may cause problems if it is less than required for the example)
        `./t4p4s.sh %l2fwd hugepages=keep`
    - Instruct `t4p4s.sh` to adjust the number of hugepages exactly to the requested amount (by default, the hugepage count is never decreased)
        `./t4p4s.sh %l2fwd hugeopt=exact`
1. Environment variables
    - Many options can be overridden using environment variables.
        `EXAMPLES_CONFIG_FILE="my_config.cfg" ./t4p4s.sh my_p4 @test`
        `EXAMPLES_CONFIG_FILE="my_config.cfg" COLOUR_CONFIG_FILE="my_colors.txt" P4_SRC_DIR="../my_files" ARCH_OPTS_FILE="my_opts.cfg" ./t4p4s.sh %my_p4 dbg verbose`
    - Running this command gives you the list of environment variables available for customisation.
    - To see which environment variables are available for customisation and what their default values are, run the following command.
        `./t4p4s.sh showenvs`
    - If `showenvs` is not the first argument, it prints the argument values after they have been fully computed/substituted
        `./t4p4s.sh %l2fwd showenvs`
1. Controller
    - Set the controller manually
        `./t4p4s.sh :l2fwd ctr=l2fwd`
    - Let the output of the controller be shown in a separate window. For this to work, `gnome-terminal` is used, as the more general `x-terminal-emulator` does not seem to work properly.
        `./t4p4s.sh %my_p4 ctrterm`
1. Miscellaneous options
    - Specify the P4 version manually (usually decided by other options or P4 file extension)
        `./t4p4s.sh :l2fwd vsn=14`
    - Pass a test option to the P4 compiler. This defines a macro called `T4P4S_TEST_1` that is available during P4 preprocessing.
        `./t4p4s.sh %my_p4 p4testcase=1`

### Testing

As described above, you can run individual test cases.
To see detailed output about compilation and execution, use the following options.

    ./t4p4s.sh %%l2fwd=payload

To run all available test cases, execute `./run_tests.sh`.
You can also give this script any number of additional options.

    ./run_tests.sh verbose dbg stats

As its name implies, `run_tests.sh` runs each test case in the offline (`nicoff`, meaning no NIC present) mode.
You may set the `PREFIX` and `POSTFIX` environment variables to make the script start `t4p4s.sh` with a different setup for the test case.
For example, the following command tests whether the test cases compile in the online (`nicon`) mode, but it doesn't execute them.

    PREFIX=: POSTFIX="" ./run_tests.sh ^run

Once the test cases are run, the script prints a summary of successful and failed test cases,
grouped by the types of failures.
You may indicate which tests are to be skipped by listing them in a file.
See the default skip file, `tests_to_skip.txt`, for further details.

    SKIP_FILE="my_skip_file" ./run_tests.sh verbose dbg


# Using Docker with T₄P₄S

You can also run `t4p4s-docker.sh` to run T₄P₄S in a Docker container.

- Docker Community Edition has to be configured on your system.
    - Usually it is available once you install the package `docker.io`.
    - For more details, see [this guide](https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/).
- Running `t4p4s-docker.sh` sets up two containers called `t4p4s` and `t4p4s-sh`.
    - Both are usable separately.
    - The `t4p4s-sh` container also takes all arguments for `t4p4s-docker.sh`.
        - For example, you can run `./t4p4s-docker.sh verbose dbg %l2fwd=payload`
        - Currently, the containers are run without a network card configuration.
- The Docker instances rely on having the same version for `linux-headers` as the host system. See the `FROM` clause in the `t4p4s-16.docker` file.
- The configuration is based on that of [`docker-dpdk` by Jeremy Eder](https://github.com/jeremyeder/docker-dpdk/), which includes using the host's `hugepages` inside the Docker instances.
    - Make sure you have enough `hugepages` on the host before running the containers.


# Working with the compiler

## HLIR

For more details on how to work with HLIR attribures, see [the readme of the `hlir16` submodule](src/hlir16/README.md).

## Special markers

The compiler uses the `.py` files inside the `hardware_indep` directory to generate Python code (saved with the extension `.desugared.py` under `build/util/desugared_compiler`), then executes the code to produce `.c` files. Under `src/utils`, files with the extension `.sugar.py` are also primarily used as code generators. The files are written with some syntactical sugar, which is described in the following.

- The files under `hardware_indep` have access to the global variable `hlir16`, which is the root of the representation.
    - The compiler silently prepares a `generated_code` global variable that starts out with an empty text. Usually, you do not want to manipulate it directly.
    - The files may contain the following markers. `PyExpr` stands for a Python expression.
        - `#[ (insert generated code here)`: the code will be textually added to `generated_code`
        - `#[ ... $my_var ...`: the textual value of the Python variable `my_var` is inserted here
        - `#[ ... ${PyExpr} ...`: the code is evaluated, then its result will be inserted as text
        - `#= PyExpr`: the expression is evaluated, its result is inserted textually
            - an alternative to this is to use `#[ ${Python expression}`
        - `#{` and `#}`: the same as `#[`, except that code between the two will be indented one level
            - the compiler expects that all opened `#{` markers will have a proper corresponding `#}` marker
        - `$${PyExpr}` highlights the evaluated text using the default colour (`T4LIGHT_default`)
            - `$$[mycolourname]{PyExpr}` uses `T4LIGHT_mycolourname` as the colour of highlighting; these colours are defined in `lights.cfg` and must be listed in `ALL_COLOURS` of `t4p4s.sh`
            - `$$[mycolourname]{PyExpr}{text}` is the same as above, but `text` (which is just plain text) also appears in the highlighted part
            - `$$[mycolourname][text1]{PyExpr}{text}` is the same as above, but `text1` (which is just plain text) also appears in the highlighted part
        - The generated C code can also use highlighting: use `T4LIT(some text)` or `T4LIT(my header instance's name,hdrinst)`
- The following capabilities are most useful inside the `.sugar.py` files, but are used in `hardware_indep` as well.
    - Functions whose name begin with `gen_` are considered helper functions in which the above markers are usable.
        - Technically, they will have a local `generated_code` variable that starts out empty, and they will return it at the end.
        - In general, such functions will contain a single conditional with multiple clauses, with each clause generating a bit of code.
        - Usually, it's a good idea to have a function with the same name (without the `gen_` part) that calls the function.
    - To facilitate finding the corresponding generator file, the desugared (generated) files contain line hints about the original file.
        - For types and expressions, these can be made inline, e.g. `uint8_t /* codegen@123*/` means that the text `uint8_t` was generated by executing code on or around line 123 in `codegen.sugar.py` (in the directory `src/utils`).
        - Most of the code generate statements, they contain hints at the end of the line such as `... // actions@123`
        - You can control the sugar style using `file_sugar_style` and the class `SugarStyle` (in `compiler.py`), see the end of `codegen.sugar.py` for usage examples.
