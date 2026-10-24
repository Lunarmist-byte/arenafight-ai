#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include "arenafight/hardware/hardware_types.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class HardwareBoardRegistry {
public:
    static HardwareBoardRegistry& instance();

    // Query boards
    std::vector<BoardSpec> getAllBoards() const;
    std::optional<BoardSpec> getBoard(const std::string& boardId) const;
    std::optional<BoardSpec> findBoard(const std::string& query) const;

    // Query components & sensors
    std::vector<ComponentSpec> getAllComponents() const;
    std::optional<ComponentSpec> getComponent(const std::string& componentId) const;
    std::optional<ComponentSpec> findComponent(const std::string& query) const;

    // Dynamic registration
    void registerBoard(BoardSpec spec);
    void registerComponent(ComponentSpec spec);

private:
    HardwareBoardRegistry();
    void initStandardBoards();
    void initStandardComponents();

    std::map<std::string, BoardSpec> boards_;
    std::map<std::string, ComponentSpec> components_;
};

} // namespace arenafight
