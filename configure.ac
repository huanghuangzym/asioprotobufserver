#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(BOL, 3.5, customer-service@asiainfo-linkage.com)

LT_PREREQ([2.2.6])
LT_INIT(static shared)
LT_LANG([C++])
AC_PROG_LIBTOOL

AC_CONFIG_MACRO_DIR([m4])
#AC_CONFIG_SRCDIR([BolCommon/ControlRegion.h])
AM_INIT_AUTOMAKE(BOL, 3.5)

# Checks for programs.
CXXFLAGS=${CXXFLAGS--g}
AC_PROG_CXX
AC_PROG_CC

# Checks for protoc
AC_CHECK_PROG(FFPROTOC_CHECK,protoc,yes)
if test x"$FFPROTOC_CHECK" != x"yes" ; then
        AC_MSG_ERROR([Please install protoc before installing.])
fi
AC_PROG_RANLIB

# Checks for libraries.
#AC_CHECK_LIB([boost_system], [exit], [], [AC_MSG_ERROR([not found  libboost_system])])
#AC_CHECK_LIB([boost_date_time], [exit], [], [AC_MSG_ERROR([ not found libboost_date_time])])
#AC_CHECK_LIB([boost_regex], [exit], [], [AC_MSG_ERROR([ not found libboost_regex])])
#AC_CHECK_LIB([BmcoUtil], [exit], [], [AC_MSG_ERROR([ not found libBmcoUtil])])
#AC_CHECK_LIB([BmcoData], [exit], [], [AC_MSG_ERROR([ not found libBmcoData])])
#AC_CHECK_LIB([BmcoFoundation], [exit], [], [AC_MSG_ERROR([ not found libBmcoFoundation])])
#AC_CHECK_LIB([BmcoXML], [exit], [], [AC_MSG_ERROR([ not found libBmcoXML])])
#AC_CHECK_LIB([BmcoNet], [exit], [], [AC_MSG_ERROR([ not found libBmcoNet])])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h malloc.h memory.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_OUTPUT(Makefile \
          src/Makefile \
          FileServer/Makefile \
          BolPublic/Makefile \
          shmtool/Makefile \
          client/Makefile \
          test/Makefile \
          TestSuite/Makefile \
          CloudAgent/Makefile \
          KPIProcess/Makefile \
          eteapi/Makefile \
          ItmLogMod/Makefile \
          asioperthread/Makefile \
          KafkaProducer/Makefile
          )
