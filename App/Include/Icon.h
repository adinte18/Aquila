// Icon.h
#ifndef ICON_H
#define ICON_H

#include <memory>
#include "Engine/Texture2D.h"
#include "imgui.h"

namespace Editor {

    // Icon class for holding an ImGui icon and its configuration
    class Icon {
    public:
        // Constructor (private to ensure usage through Builder)
        Icon(std::shared_ptr<Engine::Texture2D> texture, ImVec2 size)
            : iconTexture(texture), iconSize(size) {}

        // Display the icon using ImGui
        bool Display() const {
            if (iconTexture && iconTexture->HasImageView()) {
                ImTextureID textureID = reinterpret_cast<ImTextureID>(iconTexture->GetDescriptorSet());
                return ImGui::ImageButton("",textureID, iconSize);
            }
            return false;
        }

    private:
        std::shared_ptr<Engine::Texture2D> iconTexture;
        ImVec2 iconSize;
    };

    // Icon Builder class to help construct Icon objects
    class IconBuilder {
    public:
        IconBuilder(ImVec2 size = ImVec2(32, 32)) : size(size) {}

        // Set the texture for the icon
        IconBuilder& setTexture(std::shared_ptr<Engine::Texture2D> texture) {
            this->texture = texture;
            return *this;
        }

        // Set the size of the icon
        IconBuilder& setSize(ImVec2 newSize) {
            size = newSize;
            return *this;
        }

        // Build the icon
        std::shared_ptr<Icon> build() {
            if (!texture) {
                // You can throw an exception or return nullptr if texture is not set.
                throw std::runtime_error("Texture must be set before building the icon.");
            }
            return std::make_shared<Icon>(texture, size);
        }

    private:
        std::shared_ptr<Engine::Texture2D> texture; // The texture for the icon
        ImVec2 size;                      // Size of the icon
    };

}

#endif // ICON_H
