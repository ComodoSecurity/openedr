# from http://stackoverflow.com/questions/34637980/cmake-sort-a-list-of-target-considering-their-dependencies

define_property(TARGET PROPERTY LINK_LIBRARIES_ALL
    BRIEF_DOCS "List of all targets, linked to this one"
    FULL_DOCS "List of all targets, linked to this one"
)

# Compute list of all target links (direct and indirect) for given library
# Result is stored in LINK_LIBRARIES_ALL target property.
function(compute_links lib)
    if(${lib}_IN_PROGRESS)
        message(FATAL_ERROR "Circular dependency for library '${lib}'")
    endif()
    # Immediately return if output property is already set.
    get_property(complete TARGET ${lib} PROPERTY LINK_LIBRARIES_ALL SET)
    if(complete)
        return()
    endif()
    # Initialize output property.
    set_property(TARGET ${lib} PROPERTY LINK_LIBRARIES_ALL "")

    set(${lib}_IN_PROGRESS 1) # Prevent recursion for the same lib

    get_target_property(links ${lib} LINK_LIBRARIES)

    if(NOT links) 
        return() # Do not iterate over `-NOTFOUND` value in case of absence of the property.
    endif()

    # For each direct link append it and its links
    foreach(link ${links})
        if(TARGET ${link}) # Collect only target links
            compute_links(${link})
            get_target_property(link_links_all ${link} LINK_LIBRARIES_ALL)
            set_property(TARGET ${lib} APPEND PROPERTY
                LINK_LIBRARIES_ALL ${link} ${link_links_all}
            )
        elseif(link MATCHES "$<")
            message(STATUS "Library '${lib}' uses link '${link}'.")
            message(FATAL_ERROR "Algorithm doesn't work with generator expressions.")
        endif()
    endforeach(link ${links})
    # Remove duplicates
    get_target_property(links_all ${lib} LINK_LIBRARIES_ALL)
    list(REMOVE_DUPLICATES links_all)
    set_property(TARGET ${lib} PROPERTY LINK_LIBRARIES_ALL ${links_all})
endfunction()

# Sort given list of targets, so for any target its links come before the target itself.
#
# Uses selection sort (stable).
function(sort_links targets_list)
    # Special case of empty input list. Futher code assumes list to be non-empty.
    if(NOT ${targets_list})
        return()
    endif()

    foreach(link ${${targets_list}})
        compute_links(${link})
    endforeach()

    set(output_list)
    set(current_input_list ${${targets_list}})

    list(LENGTH current_input_list current_len)
    while(NOT current_len EQUAL 1)
        # Assume first element as minimal
        list(GET current_input_list 0 min_elem)
        set(min_index 0)
        get_target_property(min_links ${min_elem} LINK_LIBRARIES_ALL)
        # Check that given element is actually minimal
        set(index 0)
        foreach(link ${current_input_list})
            if(index) # First iteration should always fail, so skip it.
                list(FIND min_links ${link} find_index)
                if(NOT find_index EQUAL "-1")
                    # Choose linked library as new minimal element.
                    set(min_elem ${link})
                    set(min_index ${index})
                    get_target_property(min_links ${min_elem} LINK_LIBRARIES_ALL)
                endif(NOT find_index EQUAL "-1")
            endif()
            math(EXPR index "${index}+1")
        endforeach(link ${current_input_list})
        # Move minimal element from the input list to the output one.
        list(APPEND output_list ${min_elem})
        list(REMOVE_AT current_input_list ${min_index})
        math(EXPR current_len "${current_len}-1")
    endwhile()
    # Directly append the only element in the current input list to the resulted variable.
    set(${targets_list} ${output_list} ${current_input_list} PARENT_SCOPE)
endfunction(sort_links)
