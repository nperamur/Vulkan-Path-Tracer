
#include "ModelLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "glm/vec3.hpp"

struct Vec3Hash {
    size_t operator()(const glm::ivec3& v) const {
        size_t seed = 0;
        seed ^= std::hash<int>()(v.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>()(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>()(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

Model ModelLoader::load(std::string name, Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice) {
    auto oldSync = std::ios::sync_with_stdio(false);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::ifstream file("resources/" + name + ".obj");
    std::string line;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<float> finalVertices;
    std::vector<float> finalNormals;
    std::optional<std::vector<uint32_t>> indices = std::vector<uint32_t>{};
    std::unordered_map<glm::ivec3, int, Vec3Hash> indexHashes;
    while (std::getline(file, line)) {
        if (line.starts_with("vn")) {
            float x, y, z;
            sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z);
            normals.push_back({x, y, z});
        } else if (!line.starts_with("vt") && !line.starts_with("vn") && line.starts_with("v")) {
            float x, y, z;
            sscanf(line.c_str(), "v %f %f %f", &x, &y, &z);
            vertices.push_back({x, y, z});
        } else if (line.starts_with("f")) {
            int a, b, c, ta, tb, tc, na, nb, nc;
            sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &a, &ta, &na, &b, &tb, &nb, &c, &tc, &nc);
            glm::ivec3 faces[3] = {
                {a - 1, ta - 1, na - 1},
                {b - 1, tb - 1, nb - 1},
                {c - 1, tc - 1, nc - 1}
            };
            for (int i = 0; i < 3; i++) {
                if (!indexHashes.contains(faces[i])) {
                    indexHashes[faces[i]] = indexHashes.size();
                    finalVertices.push_back(vertices[faces[i].x].x);
                    finalVertices.push_back(vertices[faces[i].x].y);
                    finalVertices.push_back(vertices[faces[i].x].z);
                    finalNormals.push_back(normals[faces[i].z].x);
                    finalNormals.push_back(normals[faces[i].z].y);
                    finalNormals.push_back(normals[faces[i].z].z);
                }
                indices->push_back(indexHashes.at(faces[i]));
            }
        }
    }
    std::ios::sync_with_stdio(oldSync);
    return loader.load(finalVertices, indices, finalNormals, std::nullopt, device, physicalDevice);
}


