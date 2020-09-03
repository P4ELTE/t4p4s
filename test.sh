
declare -A OPTS
# OPTS[ports]=abc
OPTS[silent]=xyz
# OPTS[silent]=xyz

FIRSTITEM=1
COLLECTEDGREP=""
for key in ${!OPTS[@]}; do
    # [ $FIRSTITEM == 1 ] && COLLECTEDGREP="$key" && FIRSTITEM=0 && continue
    COLLECTEDGREP="$COLLECTEDGREP\|$key"
done

cat opts_dpdk.cfg | grep -ve "^\([ ]*;.*\)\?$" | grep -ve "\(DUMMYFORGREP$COLLECTEDGREP\)"
