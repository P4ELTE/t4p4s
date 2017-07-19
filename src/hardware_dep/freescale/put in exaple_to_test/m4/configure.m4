##########################################################################
# Enable/disable test-example
##########################################################################
test_example=no
AC_ARG_ENABLE([test-example],
    [  --enable-test-example   run basic test against examples],
    [if test "x$enableval" = "xyes"; then
        test_example=yes
     else
        test_example=no
    fi])

AC_CONFIG_FILES([example/classifier/Makefile
		 example/generator/Makefile
		 example/ipsec/Makefile
		 example/ipsec_proto/Makefile
		 example/Makefile
		 example/packet/Makefile
		 example/time/Makefile
		 example/timer/Makefile
		 example/reflector/Makefile
		 example/l2fwd_simple/Makefile
		 example/l3fwd/Makefile
		 example/lpm-ipfwd/Makefile
		 example/lpm-ipfwd/lpmlib/Makefile
		 example/lpm-ipfwd/app/Makefile
		 example/lpm-ipfwd/app_config/Makefile
                 example/p4_new/Makefile
		 example/switch/Makefile])
AM_COND_IF([BUILD_DPAA2], [AC_CONFIG_FILES([example/kni_demo/Makefile
		 example/cmdif_demo/Makefile
		])])
