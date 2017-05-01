#include "glsandbox/resource_database.hpp"
#include "glsandbox/resource_loader.hpp"
#include "glsandbox/log.hpp"

#include <iostream>

using namespace glsandbox;

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cout << "Usage: load_file <file_path>\n";
    return 1;
  }
 
  log_init();
  resource_database_init();
  attach_logstream(default_logstream_file_callback);
  attach_logstream(default_logstream_tail_callback);

  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(argv[1], &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);

  detach_logstream(default_logstream_tail_callback);
  detach_logstream(default_logstream_file_callback);

  return 0;
}
