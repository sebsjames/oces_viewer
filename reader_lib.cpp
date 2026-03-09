#include <cstdint>
#define OCES_READER_IMPLEMENTATION 1
#include <oces/reader>
template class oces::get_buffer<uint32_t>;
template class oces::get_buffer<uint16_t>;
template class oces::get_buffer<float>;
template class oces::get_buffer<sm::vec<float, 3>>;
