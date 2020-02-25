add_library(mediaGraphTypes
            binary_serializer.cpp
            binary_serializer.h
            string_serializer.cpp
            string_serializer.h
            type_definition.h
           )
    set_property(TARGET mediaGraphTypes PROPERTY FOLDER "mediaGraph/types")


cxx_test(binary_serializer_test binary_serializer_test.cpp
         mediaGraphTypes)
    set_property(TARGET binary_serializer_test PROPERTY FOLDER "mediaGraph/types")

cxx_test(string_serializer_test string_serializer_test.cpp
         mediaGraphTypes)
    set_property(TARGET string_serializer_test PROPERTY FOLDER "mediaGraph/types")
