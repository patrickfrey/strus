# base on http://thomasfischer.biz/find-out-the-linux-distribution-and-version-in-cmake/
# adapted for systemd's /etc/os-release and native Windows

if(NOT WIN32)
# use newer systemd stuff for platforms already supporting it
EXECUTE_PROCESS(
  COMMAND cat /etc/os-release
  COMMAND grep ^ID=
  COMMAND awk -F= "{ print $2 }"
  COMMAND tr "\n" " "
  COMMAND sed "s/ //"
  COMMAND sed "s/^\"//"
  COMMAND sed "s/\"$//"
  OUTPUT_VARIABLE SYSTEMD_ID
  RESULT_VARIABLE SYSTEMD_ID_RESULT
)
EXECUTE_PROCESS(
  COMMAND cat /etc/os-release
  COMMAND grep ^VERSION_ID=
  COMMAND awk -F= "{ print $2 }"
  COMMAND tr "\n" " "
  COMMAND sed "s/ //"
  COMMAND sed "s/^\"//"
  COMMAND sed "s/\"$//"
  OUTPUT_VARIABLE SYSTEMD_VER
  RESULT_VARIABLE SYSTEMD_VER_RESULT
)

# use the LSB stuff if possible as fallback
EXECUTE_PROCESS(
  COMMAND cat /etc/lsb-release
  COMMAND grep DISTRIB_ID
  COMMAND awk -F= "{ print $2 }"
  COMMAND tr "\n" " "
  COMMAND sed "s/ //"
  OUTPUT_VARIABLE LSB_ID
  RESULT_VARIABLE LSB_ID_RESULT
)
EXECUTE_PROCESS(
  COMMAND cat /etc/lsb-release
  COMMAND grep DISTRIB_RELEASE
  COMMAND awk -F= "{ print $2 }"
  COMMAND tr "\n" " "
  COMMAND sed "s/ //"
  OUTPUT_VARIABLE LSB_VER
  RESULT_VARIABLE LSB_VER_RESULT
)

# TODO: use distro-specific files like /etc/redhat-release

#message("SYSTEMD output: ${SYSTEMD_ID_RESULT}:${SYSTEMD_ID} ${SYSTEMD_VER_RESULT}:${SYSTEMD_VER}")
#message("LSB output: ${LSB_ID_RESULT}:${LSB_ID} ${LSB_VER_RESULT}:${LSB_VER}")

if(NOT ${SYSTEMD_ID} STREQUAL "")
  if("${SYSTEMD_VER}" STREQUAL "")
    set(SYSTEMD_VER "current")
  endif("${SYSTEMD_VER}" STREQUAL "")
  set(INSTALLER_PLATFORM "${SYSTEMD_ID}-${SYSTEMD_VER}" CACHE PATH "Installer chosen platform")
else(NOT ${SYSTEMD_ID} STREQUAL "")
if(NOT ${LSB_ID} STREQUAL "")
  # found some, use it :D
  set(INSTALLER_PLATFORM "${LSB_ID}-${LSB_VER}" CACHE PATH "Installer chosen platform")
else(NOT ${LSB_ID} STREQUAL "")
  set(INSTALLER_PLATFORM "linux-generic" CACHE PATH "Installer chosen platform")
endif(NOT ${LSB_ID} STREQUAL "")
endif(NOT ${SYSTEMD_ID} STREQUAL "")

else(NOT WIN32)
set(INSTALLER_PLATFORM "windows-generic" CACHE PATH "Installer chosen platform")
endif(NOT WIN32)
