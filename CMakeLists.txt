include(dciIntegrationUnit)
dciIntegrationUnit(module-www
        IDL_DIRS idl
        DEPENDS host)

file(GLOB_RECURSE SRC src/*)
file(GLOB_RECURSE IDL idl/*)
file(GLOB_RECURSE TST test/*)

add_library(${UNAME} MODULE ${SRC} ${IDL})
target_include_directories(${UNAME} PRIVATE src)

##############################################################
include(dciIdl)
dciIdl(${UNAME} cpp
    INCLUDE ${DCI_IDL_DIRS}
    SOURCES www.idl net.idl
)

dciIdl(${UNAME} cpp
    INCLUDE ${DCI_IDL_DIRS}
    SOURCES www.idl
    OPTIONS --cpp-no-entities --cpp-stiac-support --cpp-stiac-support-filter "^dci::idl::gen::www::"
    NAME www-stiac-support
)

##############################################################
include(dciUtilsPch)
dciUtilsPch(${UNAME} src/pch.hpp ${SRC})

############################################################
include(dciHostModule)
dciHostModule(${UNAME})

##############################################################
dciTest(${UNAME} mstart
    SRC
        ${TST}
    LINK
        host-lib
        bytes
        sbs
        exception
        mm
        cmt
        poll
        idl
    DEPENDS
        ${UNAME}
)

if(TARGET ${UNAME}-test-mstart)
    dciIdl(${UNAME}-test-mstart cpp
        INCLUDE ${DCI_IDL_DIRS}
        SOURCES www.idl net.idl
    )
endif()
