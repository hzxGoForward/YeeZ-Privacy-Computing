
add_definitions(-DHPDA_DEBUG)

add_library(hpda src/extractor/internal/https_request.cpp
  src/extractor/paged_https_extractor.cpp)
target_link_libraries(hpda pthread ssl crypto boost_system curl)

if(Release)
else(Release)
add_subdirectory(test)
#add_subdirectory(example)
endif(Release)
