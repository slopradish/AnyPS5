#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "prx/libSceGnmDriver/libSceGnmDriver.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr int InvalidArgument = static_cast<int>(0x80d11000u);
constexpr std::uint32_t Filler = 0x80000000u;

void check(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error("GnmDriver: " + what);
}

constexpr std::uint32_t Header(std::uint32_t opcode, std::uint32_t bodyWords) {
    return 0xc0000000u | ((bodyWords - 1) << 16) | (opcode << 8);
}

bool WholePackets(const std::uint32_t* words, std::size_t count) {
    std::size_t at = 0;
    while (at < count) {
        if ((words[at] >> 30) != 3) return false;
        at += ((words[at] >> 16) & 0x3fffu) + 2;
    }
    return at == count;
}

template <typename TFunction>
bool ThrowsRuntimeError(TFunction function) {
    try {
        function();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void TestWriters() {
    std::array<std::uint32_t, 0x100> words{};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawInitDefaultHardwareState175(words.data(), 0xff) == 0 && words[0] == 0xdeadbeefu, "the draw default state wrote into a short buffer");
    check(sceGnmDrawInitDefaultHardwareState175(words.data(), 0x100) == 0x100, "the draw default state is not 256 dwords");
    check(words[0] == Header(0x28, 2) && words[1] == 0x80000000u && words[2] == 0x80000000u && words[3] == Header(0x12, 1) && words[5] == Header(0x58, 6), "the draw default state does not start with CONTEXT_CONTROL, CLEAR_STATE and ACQUIRE_MEM");
    check(WholePackets(words.data(), words.size()), "the draw default state is not whole PM4 packets");

    words.fill(0xdeadbeefu);
    check(sceGnmDispatchInitDefaultHardwareState(words.data(), 0x100) == 0x100, "the dispatch default state is not 256 dwords");
    check(words[0] == (Header(0x76, 2) | 2u) && words[1] == 0x216 && words[2] == 0xffffffffu, "the dispatch default state does not start with a compute SET_SH_REG");
    check(WholePackets(words.data(), words.size()), "the dispatch default state is not whole PM4 packets");

    const std::uint32_t regs[7] = {0x1000, 0, 0x11, 0x22, 64, 1, 1};
    const std::uint32_t highAddress[7] = {0x1000, 1, 0x11, 0x22, 64, 1, 1};
    words.fill(0);
    check(sceGnmSetCsShader(words.data(), 0x18, regs) == -1, "a compute shader was written into 24 dwords");
    check(sceGnmSetCsShader(words.data(), 0x19, highAddress) == -1, "a compute shader with a nonzero high address word was accepted");
    check(sceGnmSetCsShader(words.data(), 0x19, regs) == 0, "the compute shader was not written");
    const std::uint32_t shader[] = {Header(0x76, 3), 0x20c, 0x1000, 0, Header(0x76, 3), 0x212, 0x11, 0x22, Header(0x76, 4), 0x207, 64, 1, 1, Header(0x10, 11)};
    check(std::memcmp(words.data(), shader, sizeof(shader)) == 0 && WholePackets(words.data(), 0x19), "the compute shader packets are wrong");

    words.fill(0xdeadbeefu);
    check(sceGnmInsertPushMarker(words.data(), 5, "ab") == -1, "a push marker was written into the wrong size");
    check(sceGnmInsertPushMarker(words.data(), 6, "ab") == 0, "the push marker was not written");
    check(words[0] == Header(0x10, 5) && words[1] == 0x68750001u && std::memcmp(&words[2], "ab", 3) == 0 && WholePackets(words.data(), 6), "the push marker packet is wrong");
    check(sceGnmInsertPopMarker(words.data(), 5) == -1, "a pop marker was written into 5 dwords");
    check(sceGnmInsertPopMarker(words.data(), 6) == 0 && words[0] == Header(0x10, 5) && words[1] == 0x68750002u && WholePackets(words.data(), 6), "the pop marker packet is wrong");
}

void TestSubmitValidation() {
    std::uint32_t draw[] = {Header(0x10, 1), 0};
    std::uint32_t* dcbs[] = {draw};
    std::uint32_t* nullDcbs[] = {nullptr};
    std::uint32_t* nullCcbs[] = {nullptr};
    const std::uint32_t bytes[] = {sizeof(draw)};
    const std::uint32_t empty[] = {0};
    const std::uint32_t unaligned[] = {6};
    const std::uint32_t tooLarge[] = {0x400000};
    check(sceGnmSubmitCommandBuffers(1, nullptr, bytes, nullptr, nullptr) == InvalidArgument, "a submission without draw buffers was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, nullptr, nullptr, nullptr) == InvalidArgument, "a submission without draw sizes was accepted");
    check(sceGnmSubmitCommandBuffers(1, nullDcbs, bytes, nullptr, nullptr) == InvalidArgument, "a null draw buffer was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, empty, nullptr, nullptr) == InvalidArgument, "an empty draw buffer was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, unaligned, nullptr, nullptr) == InvalidArgument, "a draw buffer of 6 bytes was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, tooLarge, nullptr, nullptr) == InvalidArgument, "a draw buffer over the size limit was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, bytes, nullptr, bytes) == InvalidArgument, "a constant buffer size without constant buffers was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, bytes, nullCcbs, bytes) == InvalidArgument, "a null constant buffer with a size was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, bytes, dcbs, unaligned) == InvalidArgument, "a constant buffer of 6 bytes was accepted");
    check(sceGnmSubmitCommandBuffers(1, dcbs, bytes, dcbs, tooLarge) == InvalidArgument, "a constant buffer over the size limit was accepted");
    check(sceGnmSubmitAndFlipCommandBuffers(0, dcbs, bytes, nullptr, nullptr, 0, 0, 0, 0) == InvalidArgument, "a flip without command buffers was accepted");
    std::uint32_t typeZero[] = {0, 0};
    std::uint32_t* typeZeroDcbs[] = {typeZero};
    const std::uint32_t typeZeroBytes[] = {sizeof(typeZero)};
    check(ThrowsRuntimeError([&] { sceGnmSubmitCommandBuffers(1, typeZeroDcbs, typeZeroBytes, nullptr, nullptr); }), "a type-0 packet was accepted");
}

alignas(256) std::array<std::uint32_t, 4> Label{};

void TestFillerSubmit() {
    const auto label = reinterpret_cast<std::uintptr_t>(Label.data());
    const auto low = static_cast<std::uint32_t>(label);
    const auto high = static_cast<std::uint32_t>(label >> 32);
    std::uint32_t draw[] = {Filler, Header(0x86, 1), 0, Header(0x85, 1), 0, Filler, Header(0x37, 4), 0x500, low, high, 0x1234, Filler};
    std::uint32_t constant[] = {Filler, Header(0x84, 1), 0, Filler};
    std::uint32_t* dcbs[] = {draw};
    std::uint32_t* ccbs[] = {constant};
    const std::uint32_t drawBytes[] = {sizeof(draw)};
    const std::uint32_t constantBytes[] = {sizeof(constant)};
    check(sceGnmSubmitCommandBuffers(1, dcbs, drawBytes, ccbs, constantBytes) == 0, "the submission with filler packets failed");
    AgcDriverWaitIdle_nid_postfix();
    check(Label[0] == 0x1234, "the WRITE_DATA between filler packets did not land");

    std::uint32_t second[] = {Header(0x37, 4), 0x500, low, high, 0x5678, Filler};
    std::uint32_t* secondDcbs[] = {second};
    const std::uint32_t secondBytes[] = {sizeof(second)};
    const std::uint32_t noConstant[] = {0};
    check(sceGnmSubmitCommandBuffers(1, secondDcbs, secondBytes, nullptr, noConstant) == 0, "a submission with zero-sized constant buffers failed");
    AgcDriverWaitIdle_nid_postfix();
    check(Label[0] == 0x5678, "the WRITE_DATA before a trailing filler packet did not land");
    check(sceGnmSubmitDone() == 0, "sceGnmSubmitDone failed");
}

}

void TestDrawWriters() {
    std::array<std::uint32_t, 0x100> words{};
    words.fill(0xdeadbeefu);
    const std::uint32_t indexAuto[] = {0xc0012d00u, 36, 0xe0000002u, 0xc0021000u, 0, 0, 0};
    check(sceGnmDrawIndexAuto(words.data(), 6, 36, 0) == -1, "an auto index draw was written into 6 dwords");
    check(sceGnmDrawIndexAuto(words.data(), 7, 36, 2) == -1, "an auto index draw accepted a reserved flag bit");
    check(sceGnmDrawIndexAuto(words.data(), 7, 36, 0xe0000001u) == 0 && std::memcmp(words.data(), indexAuto, sizeof(indexAuto)) == 0,
        "the auto index draw packets are wrong");

    const std::uint32_t index[] = {0xc0042700u, 99, 0x56789abcu, 0x1234, 99, 0x20000000u, 0xc0021000u, 0, 0, 0};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawIndex(words.data(), 10, 99, 0x1001, 0, 0) == -1, "an index draw accepted an unaligned index address");
    check(sceGnmDrawIndex(words.data(), 10, 99, 0, 0, 0) == -1, "an index draw accepted a null index address");
    check(sceGnmDrawIndex(words.data(), 10, 99, 0x123456789abcu, 0x20000000u, 0) == 0 && std::memcmp(words.data(), index, sizeof(index)) == 0,
        "the index draw packets are wrong");

    const std::uint32_t indexOffset[] = {0xc0033501u, 12, 4, 12, 0x40000000u, 0xc0021000u, 0, 0, 0};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawIndexOffset(words.data(), 10, 4, 12, 0) == -1, "an index offset draw was written into 10 dwords");
    check(sceGnmDrawIndexOffset(words.data(), 9, 4, 12, 0x40000001u) == 0 && std::memcmp(words.data(), indexOffset, sizeof(indexOffset)) == 0,
        "the index offset draw packets are wrong");

    const std::uint32_t drawIndirect[] = {0xc0032401u, 0x20, 0x4f, 0x51, 0x20000002u, 0xc0021000u, 0, 0, 0};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawIndirect(words.data(), 9, 0x20, 7, 0, 0, 0) == -1, "an indirect draw accepted an unknown stage");
    check(sceGnmDrawIndirect(words.data(), 9, 0x20, 2, 3, 5, 0x20000001u) == 0 && std::memcmp(words.data(), drawIndirect, sizeof(drawIndirect)) == 0,
        "the indirect draw packets are wrong");

    const std::uint32_t drawIndexIndirect[] = {0xc0032501u, 0x60, 0xcd, 0, 0, 0xc0021000u, 0, 0, 0};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawIndexIndirect(words.data(), 9, 0x60, 4, 1, 0, 0x1) == 0 && std::memcmp(words.data(), drawIndexIndirect, sizeof(drawIndexIndirect)) == 0,
        "the indexed indirect draw packets are wrong");

    const std::uint32_t drawIndirectMulti[] = {0xc0052c00u, 0x30, 0x14e, 0, 6, 0x10, 0x80000002u, 0xc0021000u, 0, 0, 0};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawIndirectMulti(words.data(), 11, 0x30, 6, 6, 2, 0, 0x80000000u) == 0 && std::memcmp(words.data(), drawIndirectMulti, sizeof(drawIndirectMulti)) == 0,
        "the multi indirect draw packets are wrong");

    const std::uint32_t drawIndexIndirectMulti[] = {0xc0053800u, 0x30, 0, 0x4d, 6, 0x14, 0x60000000u, 0xc0021000u, 0, 0, 0};
    words.fill(0xdeadbeefu);
    check(sceGnmDrawIndexIndirectMulti(words.data(), 11, 0x30, 6, 0, 0, 0, 0) == -1, "an indexed multi draw accepted a vertex shader stage");
    check(sceGnmDrawIndexIndirectMulti(words.data(), 11, 0x30, 6, 2, 0, 1, 0x60000000u) == 0 && std::memcmp(words.data(), drawIndexIndirectMulti, sizeof(drawIndexIndirectMulti)) == 0,
        "the indexed multi draw packets are wrong");

    words.fill(0xdeadbeefu);
    check(sceGnmDrawInitToDefaultContextState(words.data(), 0x1f) == 0 && words[0] == 0xdeadbeefu, "the default context state was written into 31 dwords");
    check(sceGnmDrawInitToDefaultContextState(words.data(), 0x21) == 0 && words[0] == 0xdeadbeefu, "the default context state was written into 33 dwords");
    check(sceGnmDrawInitToDefaultContextState(words.data(), 0x20) == 0x20, "the default context state is not 32 dwords");
    check(words[0] == 0xc0012800u && words[17] == 0xc00d1000u && words[18] == 0, "the default context state does not start with CONTEXT_CONTROL and CLEAR_STATE");
    check(std::all_of(words.begin() + 19, words.end(), [](std::uint32_t word) { return word == 0xdeadbeefu; }), "the default context state wrote past 19 dwords");

    words.fill(0xdeadbeefu);
    check(sceGnmResetVgtControl(words.data(), 4) == -1, "the VGT reset was written into 4 dwords");
    check(sceGnmResetVgtControl(words.data(), 3) == 0 && words[0] == Header(0x79, 2) && words[1] == 0x40000258u && words[2] == 0x6d007fu,
        "the VGT reset packet is wrong");
    check(sceGnmSetVgtControl(words.data(), 3, 0x7f, 1, 1) == 0 && words[0] == Header(0x79, 2) && words[1] == 0x40000258u && words[2] == 0x75007fu,
        "the VGT control packet is wrong");
    check(sceGnmSetVgtControl(words.data(), 3, 0x100, 0, 0) == -1, "the VGT control accepted 256 primitive groups");
}

int main() {
    try {
        TestWriters();
        TestDrawWriters();
        TestSubmitValidation();
        TestFillerSubmit();
        LibcRunShutdown_nid_postfix();
        std::puts("gnm driver tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try { LibcRunShutdown_nid_postfix(); } catch (...) {}
        return 1;
    }
}
