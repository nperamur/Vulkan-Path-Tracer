## Vulkan Path Tracer
This renderer uses Vulkan's Hardware Accelerated Raytracing Pipeline to implement Monte Carlo path tracing.
Diffuse lighting uses cosine weighted importance sampling and specular uses GGX VNDF sampling to model materials ranging from rough to mirror-like.
The pathtracer is progressive, meaning you can move the camera and watch the image converge over time.


<img alt="Screenshot 2026-09-02 212653" src="https://github.com/user-attachments/assets/e3b02af2-92c5-4a4b-9e93-07cfc35ddd21" />

| | Low Roughness | High Roughness |
|---|---|---|
| **Low Metallic** | <img width="400" src="https://github.com/user-attachments/assets/9a28f8ef-3e03-4105-9e5b-0e78307e2e5e" /> | <img width="400" src="https://github.com/user-attachments/assets/a0e409d1-b378-4149-96d8-8f6da79de468" /> |
| **High Metallic** | <img width="400" src="https://github.com/user-attachments/assets/69052808-05c0-4582-b7a9-d8d2b2d778ac" /> | <img width="400" src="https://github.com/user-attachments/assets/391bcb1e-1587-468b-8b88-3b7dab81ea39" /> |


<br>


The render below further demonstrates how the path tracer handles different material properties. In the image, the teapot shows rough reflections, which appear blurrier, while the sphere shows smooth reflections. The cube demonstrates a rougher, more diffuse material with less reflectivity.

<img width="400" alt="Screenshot 2026-07-20 012948" src="https://github.com/user-attachments/assets/9fba72ff-575b-42bf-9412-5e7f45aeeaf3" />


#### Update: Added Area Lights support, textured glTF model loading, and transmission for refractive materials

Renders demonstrating these additions are shown below.

<table>
  <tr>
    <td align="center"><b>Cornell box (Area lights)</b></td>
    <td align="center"><b>Textured Sponza</b></td>
    <td align="center"><b>Cornell Box Spheres (Transmission on red-tinted glass sphere)</b></td>
  </tr>
  <tr>
    <td><img width="400" alt="Screenshot 2026-09-02 000102" src="https://github.com/user-attachments/assets/37957de1-5683-4b93-81cf-639904c3e2c8" /></td>
    <td><img height="400" alt="Screenshot 2026-09-02 212653" src="https://github.com/user-attachments/assets/e3b02af2-92c5-4a4b-9e93-07cfc35ddd21" /></td>
    <td><img width="400" alt="Screenshot 2026-08-17 185006" src="https://github.com/user-attachments/assets/81ecfd03-baa8-468d-8aaf-7bcbfabfcb5d" /></td>

  </tr>
</table>




**Importance Sampling**: I used the following techniques to reduce variance and increase the convergence speed of the render.
- Cosine-weighted diffuse sampling
- GGX VNDF sampling for reflections and transmission
- Multiple importance sampling with next event estimation, using the balance heuristic
- Throughput-based russian roulette



**Limitations**:
The specular BRDF does not account for multiple scattering within rough surfaces,
so highly rough materials lose energy and may appear darker than physically accurate.
