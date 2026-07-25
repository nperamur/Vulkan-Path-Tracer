## Vulkan Path Tracer
This renderer uses Vulkan's Hardware Accelerated Raytracing Pipeline to implement Monte Carlo path tracing.
Diffuse lighting uses cosine weighted importance sampling and specular uses GGX VNDF sampling to model materials ranging from rough to mirror-like.
The pathtracer is progressive, meaning you can move the camera and watch the image converge over time.


<img width="1056" height="957" alt="Screenshot 2026-06-03 195334" src="https://github.com/user-attachments/assets/603855ac-262e-4d97-be6e-9c7befb09b55" />

| | Low Roughness | High Roughness |
|---|---|---|
| **Low Metallic** | <img width="400" src="https://github.com/user-attachments/assets/9a28f8ef-3e03-4105-9e5b-0e78307e2e5e" /> | <img width="400" src="https://github.com/user-attachments/assets/a0e409d1-b378-4149-96d8-8f6da79de468" /> |
| **High Metallic** | <img width="400" src="https://github.com/user-attachments/assets/69052808-05c0-4582-b7a9-d8d2b2d778ac" /> | <img width="400" src="https://github.com/user-attachments/assets/391bcb1e-1587-468b-8b88-3b7dab81ea39" /> |


<br>

#### Update: Added Area Lights support and textured glTF model loading

Renders demonstrating these additions are shown below.

<table>
  <tr>
    <td align="center"><b>Pathtraced Cornell box (Area lights)</b></td>
    <td align="center"><b>Textured Sponza</b></td>
  </tr>
  <tr>
    <td><img width="400" alt="Screenshot 2026-06-28 235308" src="https://github.com/user-attachments/assets/5d405821-ca61-4eba-bfe8-d86695d8c78d" /></td>
    <td><img width="400" alt="Screenshot 2026-07-22 144710" src="https://github.com/user-attachments/assets/71a214d2-27d8-4bf8-8cae-6f7c44abbf0b" /></td>
  </tr>
</table>


**Importance Sampling**: We use the following techniques to increase the convergence speed of the render.
- Cosine-weighted diffuse sampling
- GGX VNDF sampling for specular
- Multiple importance sampling with next event estimation, using the balance heuristic


**Limitations**:
The specular BRDF does not account for multiple scattering within rough surfaces,
so highly rough materials lose energy and may appear darker than physically accurate.
