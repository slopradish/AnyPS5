#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

extern "C" {
std::uint32_t APS5_VABI sceGnmDrawInitDefaultHardwareState175(std::uint32_t* cmd, std::uint32_t size);
std::uint32_t APS5_VABI sceGnmDrawInitDefaultHardwareState350(std::uint32_t* cmd, std::uint32_t size);
std::uint32_t APS5_VABI sceGnmDispatchInitDefaultHardwareState(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmDrawIndexAuto(std::uint32_t* cmd, std::uint32_t size, std::uint32_t indexCount, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndex(std::uint32_t* cmd, std::uint32_t size, std::uint32_t indexCount, std::uint64_t indexAddress, std::uint32_t flags, std::uint32_t cachePolicy);
int APS5_VABI sceGnmDrawIndexOffset(std::uint32_t* cmd, std::uint32_t size, std::uint32_t indexOffset, std::uint32_t indexCount, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndexIndirect(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr,
    std::uint32_t flags);
int APS5_VABI sceGnmDrawIndexIndirectMulti(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint32_t stage, std::uint32_t vertexSgpr,
    std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndirect(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndirectMulti(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint32_t stage, std::uint32_t vertexSgpr,
    std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawInitToDefaultContextState(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmResetVgtControl(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmSetVgtControl(std::uint32_t* cmd, std::uint32_t size, std::uint32_t primGroupSizeMinusOne, std::uint32_t partialVsWaveMode, std::uint32_t switchOnlyOnEop);
int APS5_VABI sceGnmSetCsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs);
int APS5_VABI sceGnmSetPsShader350(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs);
int APS5_VABI sceGnmSetCsShaderWithModifier(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier);
int APS5_VABI sceGnmSetEmbeddedPsShader(std::uint32_t* cmd, std::uint32_t size, std::uint32_t shaderId, std::uint32_t cachePolicy);
int APS5_VABI sceGnmSetEmbeddedVsShader(std::uint32_t* cmd, std::uint32_t size, std::uint32_t shaderId, std::uint32_t cachePolicy);
int APS5_VABI sceGnmInsertPushMarker(std::uint32_t* cmd, std::uint32_t size, const char* marker);
int APS5_VABI sceGnmInsertPopMarker(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmSubmitCommandBuffers(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes);
int APS5_VABI sceGnmSubmitAndFlipCommandBuffers(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes,
    std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg);
int APS5_VABI sceGnmSubmitDone();
}

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
    check(sceGnmDrawInitDefaultHardwareState350(words.data(), 0xff) == 0 && words[0] == 0xdeadbeefu, "the 350 default state wrote into a short buffer");
    check(sceGnmDrawInitDefaultHardwareState350(words.data(), 0x100) == 0x100, "the 350 default state is not 256 dwords");
    check(words[0] == Header(0x28, 2) && words[3] == Header(0x12, 1) && words[5] == Header(0x58, 6) && words[12] == Header(0x76, 2) && words[13] == 0x216,
        "the 350 default state does not start with CONTEXT_CONTROL, CLEAR_STATE, ACQUIRE_MEM and SET_SH_REG");
    check(words[143] == Header(0x79, 2) && words[144] == 0x40000258u && words[145] == 0x6d007fu && words[146] == Header(0x10, 109),
        "the 350 default state does not end with the 0x40000258 config register");
    check(std::all_of(words.begin() + 147, words.end(), [](std::uint32_t word) { return word == 0; }), "the 350 default state padding is not zero");
    check(WholePackets(words.data(), words.size()), "the 350 default state is not whole PM4 packets");

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

    const std::uint32_t psRegs[12] = {0x1000, 0, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, 0x8000, 0x9000, 0xa000, 0xb000};
    const std::uint32_t psBadRegs[12] = {0x1000, 1, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, 0x8000, 0x9000, 0xa000, 0xb000};
    const std::uint32_t ps[] = {Header(0x76, 3), 8, 0x1000, 0, Header(0x76, 3), 10, 0x2000, 0x3000, Header(0x69, 3), 0x1c4, 0x4000, 0x5000,
        Header(0x69, 3), 0x1b3, 0x6000, 0x7000, Header(0x69, 2), 0x1b6, 0x8000, Header(0x69, 2), 0x1b8, 0x9000, Header(0x69, 2), 0x203, 0xa000,
        Header(0x69, 2), 0x8f, 0xb000, Header(0x10, 11), 0};
    const std::uint32_t nullPs[] = {Header(0x76, 3), 8, 0, 0, Header(0x69, 2), 0x203, 0, Header(0x69, 2), 0x8f, 0xf, Header(0x10, 0x1d), 0};
    words.fill(0xdeadbeefu);
    check(sceGnmSetPsShader350(words.data(), 0x27, psRegs) == -1 && words[0] == 0xdeadbeefu, "the 350 pixel shader was written into a short buffer");
    check(sceGnmSetPsShader350(words.data(), 0x28, psBadRegs) == -1, "the 350 pixel shader accepted a nonzero high address word");
    check(sceGnmSetPsShader350(words.data(), 0x28, psRegs) == 0 && std::memcmp(words.data(), ps, sizeof(ps)) == 0, "the 350 pixel shader packets are wrong");
    words.fill(0xdeadbeefu);
    check(sceGnmSetPsShader350(words.data(), 0x28, nullptr) == 0 && std::memcmp(words.data(), nullPs, sizeof(nullPs)) == 0, "the null 350 pixel shader packets are wrong");
    check(WholePackets(words.data(), 0x28), "the null 350 pixel shader is not whole PM4 packets");

    const std::uint32_t csRegs[7] = {0x1000, 0, 0x300, 0x3000, 0x4000, 0x5000, 0x6000};
    const std::uint32_t cs[] = {Header(0x76, 3), 0x20c, 0x1000, 0, Header(0x76, 3), 0x212, 0x40, 0x3000, Header(0x76, 4), 0x207, 0x4000, 0x5000, 0x6000,
        Header(0x10, 11), 0};
    words.fill(0xdeadbeefu);
    check(sceGnmSetCsShaderWithModifier(words.data(), 0x18, csRegs, 0) == -1 && words[0] == 0xdeadbeefu, "the modified compute shader was written into a short buffer");
    check(sceGnmSetCsShaderWithModifier(words.data(), 0x19, csRegs, 0x400) == -1, "the modified compute shader accepted a reserved modifier bit");
    check(sceGnmSetCsShaderWithModifier(words.data(), 0x19, csRegs, 0x40) == 0 && std::memcmp(words.data(), cs, sizeof(cs)) == 0,
        "the modified compute shader packets are wrong");

    words.fill(0xdeadbeefu);
    check(sceGnmSetEmbeddedVsShader(words.data(), 0x1c, 0, 0) == -1 && words[0] == 0xdeadbeefu, "the embedded vertex shader was written into a short buffer");
    check(sceGnmSetEmbeddedVsShader(words.data(), 0x1d, 1, 0) == -1, "the embedded vertex shader accepted another shader id");
    check(sceGnmSetEmbeddedVsShader(words.data(), 0x1d, 0, 1) == -1, "the embedded vertex shader accepted a nonzero cache policy");
    check(sceGnmSetEmbeddedVsShader(words.data(), 0x1d, 0, 0) == 0, "the embedded vertex shader was not written");
    check(words[0] == Header(0x76, 3) && words[1] == 0x48 && words[4] == Header(0x76, 3) && words[5] == 0x4a && words[6] == 0xc0000 && words[7] == 4,
        "the embedded vertex shader does not start with the vertex shader address and resource registers");
    const std::uint32_t vs[] = {Header(0x69, 2), 0x207, 0, Header(0x69, 2), 0x1b1, 0, Header(0x69, 2), 0x1c3, 4};
    check(std::memcmp(words.data() + 8, vs, sizeof(vs)) == 0, "the embedded vertex shader context registers are wrong");
    check(WholePackets(words.data(), 0x1d), "the embedded vertex shader is not whole PM4 packets");
    const auto vsAddress = (static_cast<std::uint64_t>(words[3]) << 40) | (static_cast<std::uint64_t>(words[2]) << 8);
    const auto* vsCode = reinterpret_cast<const std::uint32_t*>(vsAddress);
    check(vsCode[0] == 0xbeeb0387u && vsCode[10] == 0xf80008cfu && vsCode[11] == 0x03020001u && vsCode[14] == 0xbf810000u,
        "the embedded vertex shader does not point at the fullscreen vertex program");

    words.fill(0xdeadbeefu);
    check(sceGnmSetEmbeddedPsShader(words.data(), 0x27, 0, 0) == -1 && words[0] == 0xdeadbeefu, "the embedded pixel shader was written into a short buffer");
    check(sceGnmSetEmbeddedPsShader(words.data(), 0x28, 2, 0) == -1, "the embedded pixel shader accepted another shader id");
    check(sceGnmSetEmbeddedPsShader(words.data(), 0x28, 0, 0) == 0, "the embedded pixel shader was not written");
    check(words[0] == Header(0x76, 3) && words[1] == 8, "the embedded pixel shader does not start with the pixel shader address");
    const std::uint32_t embeddedPs[] = {Header(0x76, 3), 10, 0xc0000, 4, Header(0x69, 3), 0x1c4, 0, 4, Header(0x69, 3), 0x1b3, 2, 2,
        Header(0x69, 2), 0x1b6, 0, Header(0x69, 2), 0x1b8, 0, Header(0x69, 2), 0x203, 0x10, Header(0x69, 2), 0x8f, 0xf};
    check(std::memcmp(words.data() + 4, embeddedPs, sizeof(embeddedPs)) == 0, "the embedded pixel shader packets are wrong");
    check(WholePackets(words.data(), 0x28), "the embedded pixel shader is not whole PM4 packets");
    const auto psAddress = (static_cast<std::uint64_t>(words[3]) << 40) | (static_cast<std::uint64_t>(words[2]) << 8);
    const auto* psCode = reinterpret_cast<const std::uint32_t*>(psAddress);
    check(psCode[0] == 0xbeeb0383u && psCode[6] == 0xbf810000u, "the embedded pixel shader does not point at the black pixel program");
    check(sceGnmSetEmbeddedPsShader(words.data(), 0x28, 1, 0) == 0 && words[6] == 0x2000u && words[11] == 2 && words[27] == 3,
        "the depth-only embedded pixel shader packets are wrong");
    const auto depthAddress = (static_cast<std::uint64_t>(words[3]) << 40) | (static_cast<std::uint64_t>(words[2]) << 8);
    check(reinterpret_cast<const std::uint32_t*>(depthAddress)[0] == 0xbeeb0383u && reinterpret_cast<const std::uint32_t*>(depthAddress)[1] == 0x7e040280u,
        "the depth-only embedded pixel shader is not the depth-only program");

    words.fill(0xdeadbeefu);
    check(sceGnmInsertPushMarker(words.data(), 5, "ab") == -1, "a push marker was written into the wrong size");
    check(sceGnmInsertPushMarker(words.data(), 6, "ab") == 0, "the push marker was not written");
    check(words[0] == Header(0x10, 5) && words[1] == 0x68750001u && std::memcmp(&words[2], "ab", 3) == 0 && WholePackets(words.data(), 6), "the push marker packet is wrong");
    check(sceGnmInsertPopMarker(words.data(), 5) == -1, "a pop marker was written into 5 dwords");
    check(sceGnmInsertPopMarker(words.data(), 6) == 0 && words[0] == Header(0x10, 5) && words[1] == 0x68750002u && WholePackets(words.data(), 6), "the pop marker packet is wrong");
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
