﻿# CMakeList.txt: проект CMake для FileIconExtractor; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

# Добавьте источник в исполняемый файл этого проекта.
add_executable (FileIconExtractor "FileIconExtractor.cpp"    )

if (CMAKE_VERSION VERSION_GREATER 3.12)
	set_property(TARGET FileIconExtractor PROPERTY CXX_STANDARD 20)
   
 
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE")


	set(SOURCES
    FileIconExtractor.cpp  # Add your source files here
	)

 

	 
	 

endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
