## Vulkan Path Tracer
This renderer uses Vulkan's Hardware Accelerated Raytracing Pipeline to implement Monte Carlo path tracing.
Diffuse lighting uses cosine weighted importance sampling and specular uses GGX VNDF sampling to model materials ranging from rough to mirror-like.
The pathtracer is progressive, meaning you can move the camera and watch the image converge over time.

<img width="600" alt="Screenshot 2026-05-30 235029" src="https://github.com/user-attachments/assets/f8a9d4ad-52b7-4094-b9f3-900a81109b60" />

| | Low Roughness | High Roughness |
|---|---|---|
| **Low Metallic** | <img width="400" src="https://github.com/user-attachments/assets/9a28f8ef-3e03-4105-9e5b-0e78307e2e5e" /> | <img width="400" src="https://github.com/user-attachments/assets/a0e409d1-b378-4149-96d8-8f6da79de468" /> |
| **High Metallic** | <img width="400" src="https://github.com/user-attachments/assets/69052808-05c0-4582-b7a9-d8d2b2d778ac" /> | <img width="400" src="https://github.com/user-attachments/assets/391bcb1e-1587-468b-8b88-3b7dab81ea39" /> |


Limitations:
Currently this uses just a directional light so there are only hard shadows. 
Additionally, the specular BRDF does not account for multiple scattering within rough surfaces,
so highly rough materials lose energy and may appear darker than physically accurate.
