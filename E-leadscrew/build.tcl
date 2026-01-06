# Vivado Project Automation Script for Spartan Edge Accelerator Board

set project_name "E-leadscrew-Spartan"
set device "xc7s15ftgb196-1"
set output_dir "./vivado_out"

# Create project
create_project $project_name $output_dir -part $device -force

# Add source files
add_files [glob ./src/*.v]

# Add constraints
add_files -fileset constrs_1 ./src/constraints.xdc

# Set top module
set_property top top [current_fileset]

# Run Synthesis
launch_runs synth_1 -jobs 4
wait_on_run synth_1

# Run Implementation
launch_runs impl_1 -jobs 4
wait_on_run impl_1

# Generate Bitstream
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

puts "Build Complete. Bitstream generated in $output_dir/${project_name}.runs/impl_1/top.bit"
