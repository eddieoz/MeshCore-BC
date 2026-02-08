# Development Guidelines for MeshCore-BitChat Integration

## Key Principles (ALWAYS FOLLOW)

### 1. Context References
**ALWAYS consult these files before any implementation:**

- **`tasks/roadmap.md`** - Master roadmap with all 31 stories
  - Current phase, story dependencies, acceptance criteria
  - Story point tracking, phase breakdown
  
- **`tasks/ENCAPSULATION_ANALYSIS.md`** - Technical architecture
  - Encapsulation strategy details
  - Protocol format specifications
  - Why no repeater changes are needed
  - Payload size constraints and fragmentation

**Check these FIRST when starting any story.**

### 2. Python Environment Isolation
**ALWAYS use pyenv for Python isolation:**

```bash
# Check if pyenv is set up
pyenv versions

# Create local environment for this project
pyenv virtualenv 3.11.0 meshcore-bitchat
pyenv local meshcore-bitchat

# Install dependencies locally
pip install platformio
pip install -r requirements.txt  # if exists

# Verify isolation
which python
# Should show: ~/.pyenv/shims/python
```

**Never use system Python or global pip installs.**

### 3. Sequential Thinking
**ALWAYS use sequential thinking process:**

```
Step 1: Problem Analysis
  - What problem are we solving?
  - What are the constraints?
  
Step 2: Design
  - What approach should we take?
  - What are the alternatives?
  
Step 3: Implementation
  - Write tests first (TDD)
  - Implement to pass tests
  
Step 4: Verification
  - Run tests
  - Check against acceptance criteria
  - Commit when passing
```

**Document each step in code comments or commit messages.**

### 4. TDD/BDD Approach
**ALWAYS follow Test-Driven Development:**

```
1. Write BDD test (Gherkin-style scenario)
2. Run test (should fail - RED)
3. Implement minimal code to pass
4. Run test (should pass - GREEN)
5. Refactor if needed
6. Commit: "feat(storyX.Y): Description"
```

**Test File Structure:**
```cpp
// test/bitchat/test_<story_name>.cpp
/**
 * Story X.Y: Story Title
 * 
 * Feature: Feature name
 *   Scenario: Scenario description
 *     Given precondition
 *     When action
 *     Then expected result
 */
 
void test_scenario_name(void) {
    // Given: Setup
    // When: Action
    // Then: Assertions
}
```

**Acceptance Criteria MUST be met:**
- [ ] Test written before implementation
- [ ] All tests passing
- [ ] No regressions in existing tests
- [ ] Code follows MeshCore style

---

## Current Status

### Completed ✅
- **Story 1.1**: Shared BLE Server Foundation (5 tests passing)
- **Story 1.2**: MeshCore BLE Service Preservation (6 tests passing)

### In Progress 🚧
- **Story 1.3**: BitChat BLE Service Addition
  - Implement BitchatBLEService
  - Integrate with SharedBLEServer
  - Test coexistence with MeshCoreUARTService

### Next Up 📋
- Story 1.4: Connection Management
- Story 2.x: Runtime Feature Control
- Story 4.x: Channel Mapping

---

## Running Tests

```bash
# Run all tests
pio test -e native_test

# Run specific story tests (add to test_main.cpp)
pio test -e native_test --filter test_name

# Hardware test on ESP32
pio test -e esp32_test_shared_ble
```

---

## Commit Message Format

```
<type>(storyX.Y): Short description

Detailed description of what was done:
- Test scenarios added
- Implementation details
- Bug fixes

Refs: Story X.Y in roadmap.md
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `test`: Test-only changes
- `docs`: Documentation
- `refactor`: Code refactoring

---

## Architecture Reminder

```
┌─────────────────────────────────────────────────────────────┐
│                    SharedBLEServer                          │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────┐  ┌─────────────────────┐          │
│  │ MeshCoreUARTService │  │  BitchatBLEService  │          │
│  │  (PIN protected)    │  │  (Open security)    │          │
│  │  • UART UUID        │  │  • BitChat UUID     │          │
│  │  • Frame queue      │  │  • Protocol bridge  │          │
│  └─────────────────────┘  └─────────────────────┘          │
└─────────────────────────────────────────────────────────────┘
                           │
                    Encapsulation Layer
                           │
              PAYLOAD_TYPE_GRP_DATA
              [14-byte header][BitChat payload]
                           │
              Repeaters forward transparently
```

**Key Technical Decisions:**
- Use `PAYLOAD_TYPE_GRP_DATA` for encapsulation
- 14-byte header: magic + version + flags + length + fragment info
- Max 170 bytes BitChat data per packet
- Fragmentation for messages > 170 bytes
- No repeater changes needed

---

## Definition of Done

For each story:
1. [ ] BDD tests written (Gherkin scenarios)
2. [ ] Tests run and pass (RED → GREEN)
3. [ ] Implementation complete
4. [ ] No compiler warnings
5. [ ] Code committed
6. [ ] Progress documented

---

## Contact & Resources

- **Roadmap**: `tasks/roadmap.md`
- **Architecture**: `tasks/ENCAPSULATION_ANALYSIS.md`
- **Completed Stories**: `tasks/story1.1_1.2_COMPLETE.md`
- **Reference Implementation**: `refs/MeshCore-BitChat-ANALYSIS.md`
