file(GLOB SRC_LIST_loc
    mainwindow.cpp
    dirmodel.cpp
    singleimageitem.cpp
    groupedimages.cpp
    viewer.cpp
)
file(GLOB UI_LIST_loc
)
file(GLOB MOC_LIST_loc
    mainwindow.h
    dirmodel.h
    singleimageitem.h
    groupedimages.h
    viewer.h
)

set(SRC_LIST ${SRC_LIST} ${SRC_LIST_loc} PARENT_SCOPE)
set(UI_LIST ${UI_LIST}   ${UI_LIST_loc} PARENT_SCOPE)
set(MOC_LIST ${MOC_LIST} ${MOC_LIST_loc} PARENT_SCOPE)
