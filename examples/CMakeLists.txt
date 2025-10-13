# Examples CMakeLists.txt

add_executable(boolean_circuit_example
	boolean_circuit.cpp
)
target_link_libraries(boolean_circuit_example
	PRIVATE
		ranked_belief
)

add_executable(recursion_example
	recursion.cpp
)
target_link_libraries(recursion_example
	PRIVATE
		ranked_belief
)

set_target_properties(boolean_circuit_example recursion_example
	PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
)
