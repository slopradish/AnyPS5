#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_RUNTIMESWITCHES_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_RUNTIMESWITCHES_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AgcDriver::Graphics {

enum class RuntimeSwitch : std::size_t {
    NoSplitDrawBuild,
    NoBdaStaging,
    NoDispatchOverlap,
    NoHtileGpuExpansion,
    NoGpuIndirectArgs,
    NoBarrierBatch,
    NoEarlyRevalidate,
    NoBatchedStaging,
    QuietPublish,
    NoStagedOverlap,
    NoOwedRunClose,
    NoRunAtomicGuard,
    DrawInputsInPlace,
    NoRunStagedReadGuard,
    NoSmallDeviceTier,
    NoExactSizeReuse,
    NoHeldReadSkip,
    NoLaneOpenSerial,
    NoLaneTemplateEdge,
    LaneSerialize,
    NoLaneOverlap,
    NoLaneLabelScope,
    NoLaneForwardEdge,
    NoLaneLeaseWait,
    NoImageSlabs,
    NoPreparedTableCount,
    NoOverlapRangeCheck,
    NoDrawAhead,
    NoPresentFrameAge,
    NoBdaTemplates,
    NoSliceCopy,
    DeferRefreshFlush,
    NoDepthIndex,
    NoDeferredDrawEviction,
    NoIncrementalBdaRanges,
    NoReadOnlyLeaseSkip,
    NoLeanBdaTables,
    NoDepthLookupFast,
    NoRefusalMemo,
    NoDistinctWriteScans,
    NoCollectHeap,
    NoLargerSmallPools,
    DeferFlipStore,
    NoDeferImportedStores,
    FlushWaitWholeRange,
    NoRefreshLookupCache,
    PipelineDispatch,
    PipelineDispatchMisses,
    NoOwnedProofs,
    NoOwnedPrivate,
    NoHitItemHtileNote,
    GpuSourcedDrawBuffers,
    ImportSourcedCopyIn,
    NoOwnedDrawSnapshots,
    NoGpuSourceAfterWriter,
    NoOwnedVertexInputs,
    NoFlushLaneWait,
    NoOwnedEpoch,
    NewPathNoGpuDraws,
    NewPathNoGpuDispatches,
    NoIndexClampMemo,
    NoGdsGpuFill,
    NoDmaCopyBehindWrites,
    NoSliceEdgeHash,
    NoClearSnapshotReuse,
    NoGpuIndexClamp,
    NoGpuWrittenInPlace,
    NoIncrementalShadowCount,
    NoGpuSingleExpand,
    NoMovedInPlace,
    NoGpuWrittenIndexEstimate,
    NoLaneOpenWait,
    NoResidentRanges,
    NoPreparedDispatchItems,
    NoBdaDrawSplit,
    NoBdaDispatchSplit,
    NoDispatchMissAhead,
    NoWrittenShadowVramGate,
    DeferLabelReadback,
    DispatchCacheLarge,
    ResourceCacheLarge,
    Count,
};

namespace RuntimeSwitches {

const char* Name(RuntimeSwitch id);
bool Set(RuntimeSwitch id);
void Assign(RuntimeSwitch id, bool set);
std::optional<RuntimeSwitch> Find(std::string_view name);
std::uint32_t KeySalt();

}

class AbSchedule {
public:
    struct Config {
        double phaseSeconds = 0;
        double settleSeconds = 2;
        std::uint64_t startPresent = 0;
        std::optional<double> startSeconds;
        std::vector<RuntimeSwitch> switches;
    };

    enum class Event { None, Begin, Settled };

    struct Step {
        Event event = Event::None;
        std::uint64_t phase = 0;
        bool b = false;
    };

    static Config Parse(std::string_view ab, std::string_view start, std::string_view settle);

    explicit AbSchedule(Config config);

    Step AtPresent(std::uint64_t present, double seconds);

    const Config& Settings() const { return config; }

private:
    Config config;
    bool started = false;
    bool settled = false;
    std::uint64_t phase = 0;
    double phaseStart = 0;
    std::optional<double> firstSeconds;
};

bool AbConfigured();
std::vector<std::string> AbAtPresent(std::uint64_t present, double seconds);

}

#endif
