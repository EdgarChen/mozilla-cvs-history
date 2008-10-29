#! /bin/bash

HOST=$(hostname | cut -d. -f1)
DOMSUF=$(domainname 2> /dev/null)
ARCH=$(uname -s)

ulimit -c unlimited 2> /dev/null

CVSROOT=":pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot"

CVS_TRUNK="mozilla/nsprpub
mozilla/dbm
mozilla/security/dbm
mozilla/security/coreconf
mozilla/security/nss
mozilla/security/jss
-r:NSS_3_11_1_RTM:mozilla/security/nss/lib/freebl/ecl/ecl-curve.h"

CVS_STABLE="-r:NSPR_4_6_BRANCH:mozilla/nsprpub
-r:NSS_3_11_BRANCH:mozilla/dbm
-r:NSS_3_11_BRANCH:mozilla/security/dbm
-r:NSS_3_11_BRANCH:mozilla/security/coreconf
-r:NSS_3_11_BRANCH:mozilla/security/nss
-r:JSS_4_2_BRANCH:mozilla/security/jss
-r:NSS_3_11_1_RTM:mozilla/security/nss/lib/freebl/ecl/ecl-curve.h"

export NSS_ENABLE_ECC=1
export NSS_ECC_MORE_THAN_SUITE_B=1
export NSPR_LOG_MODULES="pkix:1"

NSS_BUILD_TARGET="clean nss_build_all"
JSS_BUILD_TARGET="clean all"

CVS=cvs
MAKE=gmake
AWK=awk
PATCH=patch

if [ "${ARCH}" = "SunOS" ]; then
    AWK=nawk
    PATCH=gpatch
    ARCH=SunOS/$(uname -p)
fi

MAIL=mail
TB_SERVER=tinderbox-daemon@tinderbox.mozilla.org

CYCLE_MAX=5
CYCLE_TIME=60

PORT_32_DBG=8111
PORT_32_OPT=8222
PORT_64_DBG=8333
PORT_64_OPT=8444

#### SOME DEFAULTS, CAN CHANGE LATER ####

run_bits="32 64"
run_opt="DBG OPT"
BRANCH="trunk"

#### LOCAL MACHINE SETTINGS ####

# used example configurations only
case ${HOST} in
host1) 
    JAVA_HOME_64=/opt/jdk/1.6.0_01/SunOS64
    JAVA_HOME_32=/opt/jdk/1.6.0_01/SunOS
    ;;
host2)
    run_bits="32"
    export NSS_TESTS=memleak
    NO_JSS=1
    ;;
esac

RUN_BITS="${RUN_BITS:-$run_bits}"
RUN_OPT="${RUN_OPT:-$run_opt}"
