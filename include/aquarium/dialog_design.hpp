#pragma once
#include <nlohmann/json.hpp>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace aq {
struct DesignBox {
 float x{},y{},w{},h{};
 bool operator==(const DesignBox&)const=default;
};
using DesignColor=std::array<int,4>;
enum class DialogPart {Frame,Title,Illustration,Message,Extra,Detail,Action,Close,Count};
inline constexpr std::array<const char*,8> dialogPartIds{
 "frame","title","illustration","message","extra","detail","action","close"};
struct DialogElement {
 DesignBox box;
 std::string font{"baloo"};
 float fontSize{38},stretch{1};
 DesignColor color{4,47,80,255};
 std::string asset;
 bool operator==(const DialogElement&)const=default;
};
struct DialogVariant {
 std::string title,illustration,detail,actionLabel{"Open Shop"};
 std::string prefix{"You need "},singularSuffix,pluralSuffix;
 bool operator==(const DialogVariant&)const=default;
};
struct DialogDesign {
 float maxWidth{620},safeFraction{.92f};
 std::array<DialogElement,8> elements;
 float emphasisSize{68};DesignColor emphasisColor{231,82,19,255};
 float detailExtraOffset{27},detailExtraSize{29};
 bool useShopArtwork{true};std::string shopArtwork{"general-dialog/open-shop.png"};
 DialogVariant coins,pearls;
 DialogElement& element(DialogPart part){return elements.at(static_cast<std::size_t>(part));}
 const DialogElement& element(DialogPart part)const{return elements.at(static_cast<std::size_t>(part));}
 bool operator==(const DialogDesign&)const=default;
 static DialogDesign defaults();
 static DialogDesign fromJson(const nlohmann::json&);
 nlohmann::json toJson()const;
 void validate()const;
};

// A complete validated design is the unit of both reload and undo. Readers
// retain their last valid design if a writer is interrupted or makes a mistake.
class DialogDocument {
 public:
 explicit DialogDocument(std::filesystem::path path);
 const DialogDesign& design()const{return current_;}
 const std::filesystem::path& path()const{return path_;}
 const std::string& error()const{return error_;}
 bool dirty()const{return current_!=saved_;}
 bool conflict()const{return conflict_;}
 bool canUndo()const{return cursor_>0;}
 bool canRedo()const{return cursor_+1<history_.size();}
 bool apply(DialogDesign,std::string group={});
 void endEdit(){group_.clear();}
 bool undo();bool redo();bool save();bool reload(bool discardEdits=false);
 bool poll();
 private:
 std::filesystem::path path_;DialogDesign current_,saved_;
 std::vector<DialogDesign> history_;std::size_t cursor_{};
 std::string error_,group_;bool conflict_{};
 std::filesystem::file_time_type modified_{};
};

struct DialogLayout {
 DesignBox frame;float unit{};
 std::array<DesignBox,8> elements;
 const DesignBox& element(DialogPart part)const{return elements.at(static_cast<std::size_t>(part));}
};
// Device points and safe bounds have the same meaning in Studio and the game.
DialogLayout layoutDialog(const DialogDesign&,DesignBox safeBounds,float canvasUnitsPerPoint);
}
