
# Getting started with T4P4S-16

This is an experimental compiler that is
in the process of transitioning into using P4-16,
replacing more and more P4-14 code.
It still makes use of P4-14 until all parts have been transformed.
See the [README of the previous version](README14.md).

To start working with the compiler, do the following.

1. Download `bootstrap-t4p4s.sh` and run it.
    - It prepares all necessary tools in the current directory.
    - It will ask for your password in the beginning.
    - It should work on Debian based systems, e.g. the latest LTS edition of Linux Mint or Ubuntu.
    - By default, it runs downloads in parallel. You can force it to work sequentially with `PARALLEL_INSTALL=0 ./bootstrap-t4p4s.sh`.
    - You can select a DPDK version like this: `DPDK_VERSION=16.11 ./bootstrap-t4p4s.sh` or `DPDK_VERSION=16.11 DPDK_FILEVSN=16.11.1 ./bootstrap-t4p4s.sh`.
    - If you want to download T4P4S only, make sure to get it with its submodule like this: `git clone --recursive -b t4p4s-16 https://github.com/P4ELTE/t4p4s t4p4s-16`
        - When you pull further commits, you will need to update the submodules as well: `git submodule update --init --recursive` or `git submodule update --rebase --remote`
1. Don't forget to setup your environment.
    - The variable `P4C` must point to the directory of [`p4c`](https://github.com/p4lang/p4c).
    - The variable `RTE_SDK` must point to the directory of the [`DPDK` installation](http://dpdk.org/).
    - The system has to have hugepages configured. You can use `$RTE_SDK/tools/dpdk-setup.sh`, option `Setup hugepage mappings for non-NUMA systems`.
1. Running the examples is very simple: `./t4p4s.sh ./examples/l2-switch-test.p4`.
    - Make sure that before running this command, your `P4C` variable points to your [`p4c`](https://github.com/p4lang/p4c) directory, and `RTE_SDK` points your [`DPDK`](http://dpdk.org/) directory. Both are downloaded by `bootstrap-t4p4s.sh`.
    - This command uses defaults from `dpdk_parameters.cfg` and `examples.cfg`.
    - You can override behaviour from the command line like this:

~~~
# This is the default behaviour
./t4p4s.sh launch ./examples/l2-switch-test.p4
# Run only C->executable compilation
./t4p4s.sh c ./examples/l2-switch-test.p4
# Launch an already compiled executable
./t4p4s.sh run ./examples/l2-switch-test.p4
# Compile and run a program in debug mode
./t4p4s.sh dbg ./examples/l2-switch-test.p4
# Specify P4 version explicitly
./t4p4s.sh v14 ./examples/l2-switch-test.p4
# Specify P4 version explicitly (default: from examples.cfg, or v16)
./t4p4s.sh v14 ./examples/l2-switch-test.p4
# Specify DPDK configuration explicitly
./t4p4s.sh cfg "-c 0x3 -n 4 - --log-level 3 -- -p 0x3 --config \"\\\"(0,0,0),(1,0,1)\\\"\"" ./examples/l2-switch-test.p4
~~~

At the moment, P4-16 programs are not expected to compile properly.
However, they will produce C code in the `build` directory.

~~~
./t4p4s.sh p4 $P4C/testdata/p4_16_samples/vss-example.p4
~~~


# Working with the compiler

If you set the `PDB` environment variable before running `launch.sh`,
the system will open the debugger upon encountering a runtime error.

- As the debugger opens up at the beginning,
  you have to type `c` and press `Enter` to proceed.
- Note that for the following example, `ipdb` has to be installed (e.g. via `pip`).

~~~
PDB=ipdb ./t4p4s.sh dbg ./examples/l2-switch-test.p4
~~~

Of course, you can also manually add debug triggers to the code if you wish.

~~~
import ipdb
ipdb.set_trace()
~~~

A potentially interesting location is at the end of the `set_additional_attrs` function in `hlir16.py`.
There, the compilation process will be interrupted right after
the initialisation of the (currently incomplete) standard set of attributes.
Now you can access the features of the nodes of the representation.

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
