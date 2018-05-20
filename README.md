
# Getting started with T4P4S-16

This is an experimental compiler that compiles P4-16 and P4-14 files.
An [older version of the compiler](README14.md) is also available.

To start working with the compiler, do the following.

1. Download `bootstrap-t4p4s.sh` and run it.
    - It prepares all necessary tools in the current directory.
    - It will ask for your password in the beginning.
    - It should work on Debian based systems, e.g. the latest LTS edition of Linux Mint or Ubuntu.
    - By default, it runs downloads in parallel. You can force it to work sequentially with `PARALLEL_INSTALL=0 ./bootstrap-t4p4s.sh`.
    - You can select a DPDK version: `DPDK_VERSION=16.11 ./bootstrap-t4p4s.sh` or `DPDK_VERSION=16.11 DPDK_FILEVSN=16.11.1 ./bootstrap-t4p4s.sh`.
    - If you want to download T4P4S only, make sure to get it with its submodule like this: `git clone --recursive -b t4p4s-16 https://github.com/P4ELTE/t4p4s t4p4s-16`
        - When you pull further commits, you will need to update the submodules as well: `git submodule update --init --recursive` or `git submodule update --rebase --remote`
1. Don't forget to setup your environment.
    - The variable `P4C` must point to the directory of [`p4c`](https://github.com/p4lang/p4c).
    - The variable `RTE_SDK` must point to the directory of the [`DPDK` installation](http://dpdk.org/).
    - The system has to have hugepages configured. You can use `$RTE_SDK/tools/dpdk-setup.sh`, option `Setup hugepage mappings for non-NUMA systems`.
1. Running the examples is very simple: `./t4p4s.sh examples/l2fwd.p4`.
    - Make sure that before running this command, your `P4C` variable points to your [`p4c`](https://github.com/p4lang/p4c) directory, and `RTE_SDK` points your [`DPDK`](http://dpdk.org/) directory. Both are downloaded by `bootstrap-t4p4s.sh`.
    - This command uses defaults from `dpdk_parameters.cfg` and `examples.cfg`.
    - You can override behaviour from the command line like this:

~~~
# Run an example with the default configuration
./t4p4s.sh ./examples/l2fwd.p4
# This is equivalent to the above
./t4p4s.sh launch ./examples/l2fwd.p4
# Run only C->executable compilation
./t4p4s.sh c ./examples/l2fwd.p4
# Launch an already compiled executable
./t4p4s.sh run ./examples/l2fwd.p4
# Compile and run a program, show debug info during packet processing
./t4p4s.sh dbg ./examples/l2fwd.p4
# Compile and run a program, debug the Python code
./t4p4s.sh dbgpy ./examples/l2fwd.p4
# Specify P4 version explicitly (default: from examples.cfg, or v16)
./t4p4s.sh v14 ./examples/l2fwd.p4
# Choose a variant (if the example has one other than the default)
./t4p4s.sh var myCustomVariant ./examples/l2fwd.p4
# Specify DPDK configuration manually
./t4p4s.sh cfg "-c 0x3 -n 4 --log-level 3 -- -p 0x3 --config \"\\\"(0,0,0),(1,0,1)\\\"\"" ./examples/l2fwd.p4
~~~

At this point, P4-16 programs will probably compile, and they may or may not run properly.
To see small examples, see the P4 files under `examples/p4_16_v1model/test`.

- Note: if you find yourself inside a Python prompt, which indicates an error in the compilation process, you can try ignoring it by executing `c` (`continue`).

~~~
./t4p4s.sh dbg examples/p4_16_v1model/test/test-setValid-1.p4
./t4p4s.sh dbg $P4C/testdata/p4_16_samples/vss-example.p4
~~~


# Using Docker with T4P4S

The `docker` folder contains Dockerfiles and the script `t4p4s-docker-l2.sh` that illustrates how T4P4S can be used with Docker.

- Docker Community Edition has to be configured on your system; see [this guide](https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/).
- Running `t4p4s-docker-l2.sh` sets up two containers called `t4p4s-16` and `t4p4s-16-l2fwd`. Both are usable separately; the second one also runs the L2 switch example (no network card needed).
- The Docker instances rely on having the same version for `linux-headers` as the host system. See the `FROM` clause in the `t4p4s-16.docker` file.
- The configuration is based on that of [`docker-dpdk` by Jeremy Eder](https://github.com/jeremyeder/docker-dpdk/), which includes using the host's `hugepages` inside the Docker instances. Make sure you have enough `hugepages` on the host before running the containers.


# Working with the compiler

## Debugging

If you set the `PDB` environment variable before running `launch.sh`,
the system will open the debugger upon encountering a runtime error.

- As the debugger opens up at the beginning,
  you have to type `c` and press `Enter` to proceed.
- Note that for the following example, `ipdb` has to be installed (e.g. via `pip`).

~~~
PDB=ipdb ./t4p4s.sh dbg ./examples/l2fwd.p4
~~~

Of course, you can also manually add debug triggers to the code if you wish.

~~~
import ipdb; ipdb.set_trace()
~~~

A potentially interesting location is at the end of the `set_additional_attrs` function in `hlir16.py`.
There, the compilation process will be interrupted right after
the initialisation of the (currently incomplete) standard set of attributes.
Now you can access the features of the nodes of the representation.

## Gathering data

You can search for all occurrences of a string/integer/etc.
Typically you would start at the topmost node (called `hlir16`),
but any node can be used as a starting point.

~~~
hl[TAB]
hlir16.p[TAB]
hlir16.paths_to('ethernet')
hlir16.paths_to(1234567)
~~~

The result will look something like this.

~~~
  * .declarations['Type_Header'][0]
  * .declarations['Type_Struct'][4].fields
  * .declarations['P4Parser'][0].states['ParserState'][0].components['MethodCallStatement'][0].methodCall.arguments['Member'][0].expr.type.fields
  * .declarations['P4Parser'][0].states['ParserState'][0].components['MethodCallStatement'][0].methodCall.arguments['Member'][0].member
  * .declarations['P4Parser'][0].states['ParserState'][0].components['MethodCallStatement'][0].methodCall.arguments['Member'][0].type
  * .declarations['P4Parser'][0].states['ParserState'][0].components['MethodCallStatement'][0].methodCall.typeArguments['Type_Name'][0].path
  * .declarations['P4Parser'][0].states['ParserState'][0].selectExpression.select.components['Member'][0].expr.expr.type.fields
  * .declarations['P4Parser'][0].states['ParserState'][0].selectExpression.select.components['Member'][0].expr.member
...........
~~~

You can copy-paste one of these to verify the path.

~~~
ipdb> hlir16.declarations['P4Parser'][0].states['ParserState'][0].components['MethodCallStatement'][0].methodCall.arguments['Member'][0].type
ethernet_t<Type_Header>[annotations, declid, fields, name]
~~~

You can give some options to `paths_to`.

- `print_details` shows each node that each path traverses
- `match` controls how the matching works (it is always textual)

~~~
hlir16.paths_to('intrinsic_metadata')
hlir16.paths_to('intrinsic_metadata', print_details=False, match='prefix')
hlir16.paths_to('intrinsic_metadata', match='prefix')
hlir16.paths_to('intrinsic_metadata', match='infix')
hlir16.paths_to('intrinsic_metadata', match='full')
~~~


## Attributes

The nodes get their attributes in the following ways.

1. At creation, see `p4node.py`.
	- In the debugger, enter `hlir16.common_attrs` to see them.
1. Most attributes are directly loaded from the JSON file.
	- See `load_p4` in `hlir16.py`.
	- The `.json` file is produced using the `--toJSON` option of the P4 frontend `p4test`.
	  By default, this is a temporary file that is deleted upon exit.
1. Many attributes are set in `set_additional_attrs` in `hlir16.py`.
   While the compiler is in the experimental stage,
   they may be subject to change, but once it crystallizes,
   they will be considered standard.
1. You can manually add attributes using `add_attrs`, but those will be considered non-standard,
   and will not be portable in general.

The representation contains internal nodes (of type `P4Node`)
and leaves (primitives like ints and strings).
Internal nodes will sometimes be (ordered) vectors.

Some of the more important attributes are the following.

~~~
hl[TAB].d[TAB]        # expands to...
hlir16.declarations   # these are the top-level declarations in the program

ds = hlir16.declarations
ds.is_vec()           # True
ds[0]                 # indexes the vector; the first declaration in the program
ds.b[TAB]             # expands to...
ds.by_type('Type_Struct')   # gives you all 'Type_Struct' declarations
ds.by_type('Struct')        # shortcut; many things are called 'Type_...'
ds.get('name')        # all elems in the vector with the name 'name'
ds.get('ipv4_t', 'Type_Header')   # the same, limited to the given type

any_node.name         # most nodes (but not all) have names
any_node.xdir()       # names of the node's non-common attributes
~~~


## Special markers

The compiler uses the `.py` files inside the `hardware_indep` directory to generate Python code (saved with the extension `.desugared.py` under `build/util/desugared_compiler`), then executes the code to produce `.c` files. Under `src/utils`, files with the extension `.sugar.py` are also primarily used as code generators. The files are written with some syntactical sugar, which is described in the following.

- The files under `hardware_indep` have access to the global variable `hlir16`, which is the root of the representation.
    - The compiler silently prepares a `generated_code` global variable that starts out with an empty text. Usually, you do not want to manipulate it directly.
    - The files may contain the following markers.
        - `#[ (insert generated code here)`: the code will be textually added to `generated_code`
        - `#[ ... $my_var ...`: the textual value of the Python variable `my_var` is inserted here
        - `#[ ... ${Python code} ...`: the code is evaluated, then its result will be inserted as text
        - `#= (Python expression)`: the expression is evaluated, its result is inserted textually
            - an alternative to this is to use `#[ ${Python expression}`
        - `#{` and `#}`: the same as `#[`, except that code between the two will be indented one level
            - the compiler expects that all opened `#{` markers will have a proper corresponding `#}` marker
- The following capabilities are most useful inside the `.sugar.py` files, but are used in `hardware_indep` as well.
    - Functions whose name begin with `gen_` are considered helper functions in which the above markers are usable.
        - Technically, they will have a local `generated_code` variable that starts out empty, and they will return it at the end.
        - In general, such functions will contain a single conditional with multiple clauses, with each clause generating a bit of code.
        - Usually, it's a good idea to have a function with the same name (without the `gen_` part) that calls the function.
    - To facilitate finding the corresponding generator file, the desugared (generated) files contain line hints about the original file.
        - For types and expressions, these can be made inline, e.g. `uint8_t /* codegen@123*/` means that the text `uint8_t` was generated by executing code on or around line 123 in `codegen.sugar.py` (in the directory `src/utils`).
        - Most of the code generate statements, they contain hints at the end of the line such as `... // actions@123`
        - You can control the sugar style using `file_sugar_style` and the class `SugarStyle` (in `compiler.py`), see the end of `codegen.sugar.py` for usage examples.
