#include <cstdint>
#define OCES_READER_IMPLEMENTATION 1
#include <oces/reader>
template void oces::get_buffer<float>(const tinygltf::Model&, const int32_t, std::vector<float>&);
template void oces::get_buffer<uint32_t>(const tinygltf::Model&, const int32_t, std::vector<uint32_t>&);
template void oces::get_buffer<uint16_t>(const tinygltf::Model&, const int32_t, std::vector<uint16_t>&);
//template class sm::vec<float, 3>;
//template void oces::get_buffer<float>(const tinygltf::Model&, const int32_t, std::vector<sm::vec<float, 3>>&);
