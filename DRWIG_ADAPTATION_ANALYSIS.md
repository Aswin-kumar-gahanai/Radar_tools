# DRWIG Parser Adaptation Analysis

## Current Implementation vs DRWIG Requirements

### 1. **Protocol Structure Differences** ❌ **NEEDS ADAPTATION**

#### Current AWR1843 Implementation:
```cpp
// Current magic word (AWR1843)
static constexpr uint8_t MAGIC_WORD[8] = {0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07};

// Current TLV types
static constexpr uint32_t MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1;
```

#### DRWIG Requirements:
```cpp
// DRWIG magic word (same as AWR1843 - ✅ Compatible)
constexpr std::array<uint8_t, 8> kMagicNumber{2,1,4,3,6,5,8,7};

// DRWIG TLV types (different enumeration)
enum class DrwigTlvTypes : uint32_t {
    MSG_NULL = 0,
    MSG_DETECTED_POINTS = 1,    // ✅ Compatible
    MSG_CLUSTERS = 2,           // ❌ New type needed
    MSG_TRACKED_OBJ = 3,        // ❌ New type needed
    MSG_PARKING_ASSIST = 4,     // ❌ New type needed
    MSG_RANGE_DOPPLER_HEAT_MAP = 5,
    MSG_STATISTICS = 6,
    MSG_DETECTED_POINTS_SIDE_INFO = 7,
    MSG_MAX
};
```

### 2. **Object Structure Differences** ❌ **NEEDS ADAPTATION**

#### Current Implementation:
```cpp
struct DetectedObject {
    int radar_id;
    int frame_number;
    double angle, distance, velocity;
    double x, y, z;
    uint16_t peak_val;
    uint16_t range_idx;
    int16_t doppler_idx;
};
```

#### DRWIG Requirements:
```cpp
struct DrwigDetectedObject {
    int16_t doppler_velocity;  // Fixed-point
    uint16_t peak_value;
    int16_t x;                 // Fixed-point
    int16_t y;                 // Fixed-point  
    int16_t z;                 // Fixed-point
};

struct DrwigClusterObject {    // ❌ Missing in current
    int16_t x, y;
    uint16_t width, length;
};

struct DrwigTrackedObject {    // ❌ Missing in current
    int16_t x, y;
    int16_t vx, vy;           // Velocity components
    uint16_t width, length;
};
```

### 3. **Threading Architecture** ✅ **PARTIALLY COMPATIBLE**

#### Current Implementation:
```cpp
class DataProcessor {
    std::thread worker_thread_;
    std::vector<std::shared_ptr<RingBuffer<RadarFrame>>>& buffers_;
    std::queue<RadarFrame>& output_queue_;
};
```

#### DRWIG Requirements:
```cpp
class DrwigParser {
    std::thread read_uart;     // ✅ Similar concept
    std::thread parse_data;    // ✅ Similar concept
    TSQueue<uint32_t> write_queue;
    TSQueue<uint32_t> reader_queue;
};
```

### 4. **Callback System** ❌ **MISSING**

#### Current Implementation:
- Direct queue-based communication
- No callback registration system

#### DRWIG Requirements:
```cpp
// Callback handlers needed
void RegisterObjectHandler(DrwigDetectObjectHandler callback);
void RegisterObjectHandler(DrwigClusterObjectHandler callback);
void RegisterObjectHandler(DrwigTrackedObjectHandler callback);

// Callback types
using DrwigDetectObjectHandler = std::function<void(const DrwigObjectHeader&, const std::vector<DrwigDetectedObject>&)>;
using DrwigClusterObjectHandler = std::function<void(const DrwigObjectHeader&, const std::vector<DrwigClusterObject>&)>;
using DrwigTrackedObjectHandler = std::function<void(const DrwigObjectHeader&, const std::vector<DrwigTrackedObject>&)>;
```

## Required Adaptations

### 1. **Extend TLV Type Support** 🔧

```cpp
// File: cpp/include/parsers/awr1843_parser.h
enum class TlvType : uint32_t {
    DETECTED_POINTS = 1,
    RANGE_PROFILE = 2,
    NOISE_PROFILE = 3,
    AZIMUTH_HEATMAP = 4,
    RANGE_DOPPLER_HEATMAP = 5,
    STATISTICS = 6,
    // DRWIG extensions
    CLUSTERS = 7,              // New
    TRACKED_OBJECTS = 8,       // New
    PARKING_ASSIST = 9,        // New
    DETECTED_POINTS_SIDE_INFO = 10  // New
};
```

### 2. **Add Missing Object Types** 🔧

```cpp
// File: cpp/include/common/types.h
struct ClusterObject {
    int radar_id;
    int frame_number;
    double x, y;               // Position
    double width, length;      // Dimensions
    int cluster_id;
    double confidence;
};

struct TrackedObject {
    int radar_id;
    int frame_number;
    double x, y;               // Position
    double vx, vy;             // Velocity
    double width, length;      // Dimensions
    int track_id;
    double confidence;
    int age;                   // Tracking age
};
```

### 3. **Implement Callback System** 🔧

```cpp
// File: cpp/include/core/radar_system.h
class RadarSystem {
public:
    // Callback registration
    using DetectedObjectCallback = std::function<void(const std::vector<DetectedObject>&)>;
    using ClusterObjectCallback = std::function<void(const std::vector<ClusterObject>&)>;
    using TrackedObjectCallback = std::function<void(const std::vector<TrackedObject>&)>;
    
    void register_detected_callback(DetectedObjectCallback callback);
    void register_cluster_callback(ClusterObjectCallback callback);
    void register_tracked_callback(TrackedObjectCallback callback);

private:
    DetectedObjectCallback detected_callback_;
    ClusterObjectCallback cluster_callback_;
    TrackedObjectCallback tracked_callback_;
};
```

### 4. **Enhanced Parser Implementation** 🔧

```cpp
// File: cpp/src/parsers/awr1843_parser.cpp
std::vector<DetectedObject> AWR1843Parser::parse_frame(const std::vector<uint8_t>& raw_data, int radar_id) {
    std::vector<DetectedObject> all_objects;
    
    // Parse each TLV block
    for (auto& tlv : parse_tlv_blocks(raw_data)) {
        switch (tlv.type) {
            case TlvType::DETECTED_POINTS:
                auto detected = parse_detected_objects_tlv(tlv.data, radar_id);
                all_objects.insert(all_objects.end(), detected.begin(), detected.end());
                break;
                
            case TlvType::CLUSTERS:
                auto clusters = parse_cluster_objects_tlv(tlv.data, radar_id);
                // Convert clusters to DetectedObject format or handle separately
                break;
                
            case TlvType::TRACKED_OBJECTS:
                auto tracked = parse_tracked_objects_tlv(tlv.data, radar_id);
                // Convert tracked to DetectedObject format or handle separately
                break;
        }
    }
    
    return all_objects;
}
```

### 5. **Fixed-Point Conversion** 🔧

```cpp
// File: cpp/src/parsers/awr1843_parser.cpp
class FixedPointConverter {
public:
    static constexpr double POSITION_SCALE = 0.01;  // 1 cm resolution
    static constexpr double VELOCITY_SCALE = 0.1;   // 0.1 m/s resolution
    static constexpr double SIZE_SCALE = 0.01;       // 1 cm resolution
    
    static double convert_position(int16_t fixed_point) {
        return static_cast<double>(fixed_point) * POSITION_SCALE;
    }
    
    static double convert_velocity(int16_t fixed_point) {
        return static_cast<double>(fixed_point) * VELOCITY_SCALE;
    }
};
```

## Implementation Priority

### **Phase 1: Core Compatibility** (High Priority)
1. ✅ Magic word compatibility (already compatible)
2. 🔧 Extend TLV type enumeration
3. 🔧 Add missing object structures
4. 🔧 Implement fixed-point conversion

### **Phase 2: Enhanced Features** (Medium Priority)
1. 🔧 Implement callback system
2. 🔧 Add cluster object parsing
3. 🔧 Add tracked object parsing
4. 🔧 Enhanced error handling

### **Phase 3: Advanced Features** (Low Priority)
1. 🔧 Connection recovery system
2. 🔧 Memory pool optimization
3. 🔧 Performance profiling
4. 🔧 Configuration panel integration

## Compatibility Assessment

### **✅ Compatible Elements:**
- Magic word sequence (identical)
- Basic frame structure (40-byte header)
- Threading architecture concept
- Serial communication approach
- TLV parsing methodology

### **❌ Incompatible Elements:**
- Object structure definitions
- TLV type enumeration (partially)
- Callback system (missing)
- Fixed-point conversion (missing)
- Multi-object type handling

### **🔧 Adaptation Required:**
- Extend current parser to handle DRWIG-specific TLV types
- Add missing object structures (Cluster, Tracked)
- Implement callback registration system
- Add fixed-point to floating-point conversion
- Enhance CSV logging for multiple object types

## Recommended Implementation Strategy

### **Minimal Adaptation Approach:**
1. Keep current AWR1843Parser as base class
2. Create DrwigParser as derived class with extensions
3. Add DRWIG-specific object types alongside existing ones
4. Implement backward compatibility with current CSV format
5. Add optional callback system without breaking existing queue-based approach

### **Code Structure:**
```cpp
// Maintain compatibility
class AWR1843Parser {
    // Current implementation
};

// Extend for DRWIG
class DrwigParser : public AWR1843Parser {
    // DRWIG-specific extensions
    std::vector<ClusterObject> parse_cluster_objects_tlv(...);
    std::vector<TrackedObject> parse_tracked_objects_tlv(...);
    
    // Callback system
    void register_callbacks(...);
};
```

This approach ensures **backward compatibility** while adding DRWIG-specific features incrementally.