add_library(mediaGraphTypes
            binary_serializer.cpp
            binary_serializer.h
            string_serializer.cpp
            string_serializer.h
            type_definition.h
           )
    set_property(TARGET mediaGraphTypes PROPERTY FOLDER "mediaGraph/types")


cxx_test(binary_serializer_test "mediaGraph/types" binary_serializer_test.cpp
         mediaGraphTypes)

cxx_test(string_serializer_test "mediaGraph/types" string_serializer_test.cpp
         mediaGraphTypes)
