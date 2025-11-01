// internal
#include "physics_system.hpp"
#include "world_init.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// transform polygon points from local space to world space

static void transform_polygon(const Motion& motion,
							   const std::vector<vec2>& local_points,
							   std::vector<vec2>& world_points)
{
    world_points.resize(local_points.size());
    const float cos_theta = cos(motion.angle);
	
    const float sin_theta = sin(motion.angle);
    for (size_t i = 0; i < local_points.size(); ++i) {
        // scale in local space
        vec2 scaled = { local_points[i].x * motion.scale.x,
                        local_points[i].y * motion.scale.y };
        // rotate around origin
        vec2 rotated = { scaled.x * cos_theta - scaled.y * sin_theta,
                         scaled.x * sin_theta + scaled.y * cos_theta };
        // translate to world
        world_points[i] = rotated + motion.position;
    }
}

// project a polygon onto an axis and return the min/max scalar projections
static void project_axis(const std::vector<vec2>& polygon,
						  const vec2& axis,
						  float& out_min,
						  float& out_max)
{
    float projection = polygon[0].x * axis.x + polygon[0].y * axis.y;
    out_min = projection;
    out_max = projection;
    for (size_t i = 1; i < polygon.size(); ++i) {
        projection = polygon[i].x * axis.x + polygon[i].y * axis.y;
        if (projection < out_min) out_min = projection;
		
        if (projection > out_max) out_max = projection;
    }
}


static bool check_axes_for_overlap(const std::vector<vec2>& polygon,
                                   const std::vector<vec2>& poly_a,
                                   const std::vector<vec2>& poly_b,
                                   float& smallest_overlap,
                                   vec2& best_axis)
{
    const size_t count = polygon.size();

    for (size_t i = 0; i < count; ++i) {
        const vec2 a = polygon[i];
        const vec2 b = polygon[(i + 1) % count];
        vec2 edge = { b.x - a.x, b.y - a.y };
        vec2 axis = { -edge.y, edge.x };

        const float axis_len = sqrt(axis.x * axis.x + axis.y * axis.y);
        if (axis_len <= 0.00001f) continue;
        axis.x /= axis_len; 
		axis.y /= axis_len;
        float a_min, a_max, b_min, b_max;
        project_axis(poly_a, axis, a_min, a_max);
        project_axis(poly_b, axis, b_min, b_max);

        if (a_max < b_min || b_max < a_min) return false;
        const float overlap = std::min(a_max, b_max) - std::max(a_min, b_min);
        if (overlap < smallest_overlap) { smallest_overlap = overlap; best_axis = axis; }
        }
        return true;
}

// check two convex polygons for overlap using SAT. returns true if overlapping
static bool sat_overlap(const std::vector<vec2>& poly_a,
					   const std::vector<vec2>& poly_b,
					   vec2& out_mtv)
{
    float smallest_overlap = 999999.0f;
    vec2 best_axis = { 0.f, 0.f };

    if (!check_axes_for_overlap(poly_a, poly_a, poly_b, smallest_overlap, best_axis)) return false;
    if (!check_axes_for_overlap(poly_b, poly_a, poly_b, smallest_overlap, best_axis)) return false;

    out_mtv = { best_axis.x * smallest_overlap, best_axis.y * smallest_overlap };
    return true;
}

// check overlap between a convex polygon and a circle using SAT
static bool sat_polygon_circle(const std::vector<vec2>& polygon,
							 const vec2& circle_center,
							 float circle_radius,
							 vec2& out_mtv)
{
    float smallest_overlap = 99999.0f;
    vec2 best_axis = { 0.f, 0.f };

    // test all polygon edge normals
    for (size_t i = 0; i < polygon.size(); ++i) {
        const vec2 p1 = polygon[i];
        const vec2 p2 = polygon[(i + 1) % polygon.size()];
        const vec2 edge = { p2.x - p1.x, p2.y - p1.y };
        vec2 axis = { -edge.y, edge.x };

        const float len = sqrt(axis.x * axis.x + axis.y * axis.y);
        if (len < 0.00001f) continue;
        axis.x /= len; axis.y /= len;
        float min_p, max_p;

        project_axis(polygon, axis, min_p, max_p);
        const float center_proj = circle_center.x * axis.x + circle_center.y * axis.y;
        const float min_c = center_proj - circle_radius;

        const float max_c = center_proj + circle_radius;
        if (max_p < min_c || max_c < min_p) return false;
        const float overlap = std::min(max_p, max_c) - std::max(min_p, min_c);

        if (overlap < smallest_overlap) { smallest_overlap = overlap; best_axis = axis; }
    }

    // test axis from circle center to closest vertex
    vec2 closest = polygon[0];
    float best_dist2 = dot(vec2{ circle_center.x - closest.x, circle_center.y - closest.y },
                           vec2{ circle_center.x - closest.x, circle_center.y - closest.y });

    for (size_t i = 1; i < polygon.size(); ++i) {
        vec2 d = { circle_center.x - polygon[i].x, circle_center.y - polygon[i].y };

        float d2 = dot(d, d);
        if (d2 < best_dist2) { best_dist2 = d2; closest = polygon[i]; }
    }

    vec2 axis_to_vertex = { circle_center.x - closest.x, circle_center.y - closest.y };
    float axis_len = sqrt(axis_to_vertex.x * axis_to_vertex.x + axis_to_vertex.y * axis_to_vertex.y);
    if (axis_len > 0.00001f) {
        axis_to_vertex.x /= axis_len;
        axis_to_vertex.y /= axis_len;

        float min_p, max_p;
        project_axis(polygon, axis_to_vertex, min_p, max_p);

        const float center_proj = circle_center.x * axis_to_vertex.x + circle_center.y * axis_to_vertex.y;
        const float min_c = center_proj - circle_radius;

        const float max_c = center_proj + circle_radius;
        if (max_p < min_c || max_c < min_p) return false;
        const float overlap = std::min(max_p, max_c) - std::max(min_p, min_c);
        if (overlap < smallest_overlap) { smallest_overlap = overlap; best_axis = axis_to_vertex; }
    }

    out_mtv = { best_axis.x * smallest_overlap, best_axis.y * smallest_overlap };
    return true;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	for(uint i = 0; i< motion_registry.size(); i++)
	{
		// Update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		Entity e = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
		
        // Update position based on velocity and time elapsed
        vec2 new_pos = motion.position + motion.velocity * step_seconds;
		if (registry.constrainedEntities.has(e)) {
			vec2 camera_pos = {0.f, 0.f};
			for (Entity player_entity : registry.players.entities) {
				if (registry.motions.has(player_entity)) {
					camera_pos = registry.motions.get(player_entity).position;
					break;
				}
			}
			
			vec2 bbox = get_bounding_box(motion);
			float half_window_width = (float) window_width_px / 2.0f;
			float half_window_height = (float) window_height_px / 2.0f;
			
			float min_x = camera_pos.x - half_window_width + (bbox.x / 2);
			float max_x = camera_pos.x + half_window_width - (bbox.x / 2);
			float min_y = camera_pos.y - half_window_height + (bbox.y / 2);
			float max_y = camera_pos.y + half_window_height - (bbox.y / 2);

			if (new_pos.x < min_x)
				new_pos.x = min_x;
			else if (new_pos.x > max_x)
				new_pos.x = max_x;

			if (new_pos.y < min_y)
				new_pos.y = min_y;
			else if (new_pos.y > max_y)
				new_pos.y = max_y;
		}
        
        motion.position = new_pos;
	}

    // trees are static, they block dynamics entities and despawn bullets
    for (Entity obs_e : registry.obstacles.entities)
    {
        if (!registry.motions.has(obs_e)) continue;
        Motion& obs_m = registry.motions.get(obs_e);
        const bool obs_has_mesh = registry.colliders.has(obs_e);
        const bool obs_has_circ = registry.collisionCircles.has(obs_e);

        for (Entity dyn_e : registry.motions.entities)
        {
            if (dyn_e == obs_e || registry.obstacles.has(dyn_e) || registry.feet.has(dyn_e)) continue;
            Motion& dyn_m = registry.motions.get(dyn_e);
            const bool dyn_has_mesh = registry.colliders.has(dyn_e);
            const bool dyn_has_circ = registry.collisionCircles.has(dyn_e);

            auto radius_of = [&](Entity e, const Motion& m) {
                if (registry.collisionCircles.has(e))
                    return registry.collisionCircles.get(e).radius;
                vec2 half_bb = get_bounding_box(m) / 2.f;
                return sqrtf(dot(half_bb, half_bb));
            };

            if (registry.bullets.has(dyn_e))
            {
                const float bullet_r = radius_of(dyn_e, dyn_m);
                bool hit = false;
                if (obs_has_mesh) {
                    std::vector<vec2> obs_poly;
                    transform_polygon(obs_m, registry.colliders.get(obs_e).local_points, obs_poly);
                    vec2 mtv;
                    hit = sat_polygon_circle(obs_poly, dyn_m.position, bullet_r, mtv);
                } else {
                    
                    const float obs_r = radius_of(obs_e, obs_m);
                    vec2 dp = dyn_m.position - obs_m.position;
                    hit = dot(dp, dp) < (bullet_r + obs_r) * (bullet_r + obs_r);
                }
                if (hit) {
                    registry.remove_all_components_of(dyn_e);
                    continue;
                }
                continue;
            }

            bool is_bonfire = false;
            if (registry.renderRequests.has(obs_e)) {
                RenderRequest& req = registry.renderRequests.get(obs_e);
                if (req.used_texture == TEXTURE_ASSET_ID::BONFIRE) {
                    is_bonfire = true;
                }
            }
            
            bool is_player = registry.players.has(dyn_e);
            if (is_player && is_bonfire) {
                continue;
            }
            
            bool blocked = false;
            vec2 push = { 0.f, 0.f };

            if (dyn_has_circ && obs_has_mesh)
            {
                std::vector<vec2> obs_poly;
                transform_polygon(obs_m, registry.colliders.get(obs_e).local_points, obs_poly);
                vec2 mtv;
                if (sat_polygon_circle(obs_poly, dyn_m.position, registry.collisionCircles.get(dyn_e).radius, mtv))
                {
                    vec2 from_obs_to_dyn = dyn_m.position - obs_m.position;
                    float length = sqrtf(dot(from_obs_to_dyn, from_obs_to_dyn));
                    if (length > 0.00001f)
                    {
                        vec2 dir = { from_obs_to_dyn.x / length, from_obs_to_dyn.y / length };
                        float mtv_len = sqrtf(dot(mtv, mtv));
                        push = { dir.x * mtv_len, dir.y * mtv_len };
                        blocked = true;
                    }
                }
            }
            else if (dyn_has_mesh && obs_has_circ)
            {
                std::vector<vec2> dyn_poly;
                transform_polygon(dyn_m, registry.colliders.get(dyn_e).local_points, dyn_poly);
                vec2 mtv;
                if (sat_polygon_circle(dyn_poly, obs_m.position, registry.collisionCircles.get(obs_e).radius, mtv))
                {
                    push = { -mtv.x, -mtv.y };
                    blocked = true;
                }
            }
            else if (dyn_has_mesh && obs_has_mesh)
            {
                std::vector<vec2> obs_poly, dyn_poly;
                transform_polygon(obs_m, registry.colliders.get(obs_e).local_points, obs_poly);
                transform_polygon(dyn_m, registry.colliders.get(dyn_e).local_points, dyn_poly);
                vec2 mtv;
				
                if (sat_overlap(dyn_poly, obs_poly, mtv))
                {
                    push = mtv;
                    blocked = true;
                }
            }
            else
            {
                vec2 dp = dyn_m.position - obs_m.position;
                float ro = radius_of(obs_e, obs_m);
                float rd = radius_of(dyn_e, dyn_m);
                float dist2 = dot(dp, dp);
                float sum = rd + ro;
                if (dist2 < sum * sum)
                {
                    float dist = sqrtf(std::max(dist2, 0.00001f));
                    vec2 n = { dp.x / dist, dp.y / dist };
                    float overlap = sum - dist;
                    push = { n.x * overlap, n.y * overlap };
                    blocked = true;
                }
            }

            if (blocked)
            {
                dyn_m.position += push;
                float push_len = sqrtf(dot(push, push));
                if (push_len > 0.00001f)
                {
                    vec2 n = { push.x / push_len, push.y / push_len };
                    float vn = dyn_m.velocity.x * n.x + dyn_m.velocity.y * n.y;
                    if (vn < 0.f)
                    {
                        dyn_m.velocity.x -= n.x * vn;
                        dyn_m.velocity.y -= n.y * vn;
                    }
                }
            }
        }
    }

	// Check for collisions between all moving entities
    ComponentContainer<Motion> &motion_container = registry.motions;
	for(uint i = 0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		
		for(uint j = i+1; j<motion_container.components.size(); j++)
		{
            Motion& motion_j = motion_container.components[j];
            Entity entity_j = motion_container.entities[j];

            const bool has_col_i = registry.colliders.has(entity_i);
            const bool has_col_j = registry.colliders.has(entity_j);
            const bool has_circ_i = registry.collisionCircles.has(entity_i);
            const bool has_circ_j = registry.collisionCircles.has(entity_j);
            const bool is_bullet_i = registry.bullets.has(entity_i);
            const bool is_bullet_j = registry.bullets.has(entity_j);
            const bool is_player_i = registry.players.has(entity_i);
            const bool is_player_j = registry.players.has(entity_j);
            const bool is_feet_i = registry.feet.has(entity_i);
            const bool is_feet_j = registry.feet.has(entity_j);
            if (is_feet_i || is_feet_j) continue;
            if (registry.obstacles.has(entity_i) || registry.obstacles.has(entity_j)) continue;

            auto radius_from_motion = [&](const Motion& motion) {
                vec2 half_bb = get_bounding_box(motion) / 2.f;
                return sqrtf(dot(half_bb, half_bb));
            };

            auto radius_of = [&](Entity entity, const Motion& motion) {
                if (registry.collisionCircles.has(entity))
                    return registry.collisionCircles.get(entity).radius;
                return radius_from_motion(motion);
            };

            auto circle_circle_overlap = [&](const Motion& a_motion, float a_radius,
                                             const Motion& b_motion, float b_radius) {
                vec2 delta = a_motion.position - b_motion.position;
                float distance_sq = dot(delta, delta);
                float sum_radii = a_radius + b_radius;
                return distance_sq < sum_radii * sum_radii;
            };

            auto poly_from = [&](Entity entity, const Motion& motion) {
                std::vector<vec2> world_points;
                transform_polygon(motion, registry.colliders.get(entity).local_points, world_points);
                return world_points;
            };

            // damage detection
            bool hit_for_damage = false;
            const bool is_enemy_i = registry.enemies.has(entity_i);
            const bool is_enemy_j = registry.enemies.has(entity_j);

            if (has_col_i && has_col_j) {
                auto pi = poly_from(entity_i, motion_i);
                auto pj = poly_from(entity_j, motion_j);
                vec2 mtv;
                bool overlap = sat_overlap(pi, pj, mtv);
                if (overlap)
                    hit_for_damage = true;
            }
            else if (has_col_i && has_circ_j) {
                auto pi = poly_from(entity_i, motion_i);
                float rj = registry.collisionCircles.get(entity_j).radius;
                vec2 mtv;
                bool overlap = sat_polygon_circle(pi, motion_j.position, rj, mtv);
                if (overlap)
                    hit_for_damage = true;
            }
            else if (has_col_j && has_circ_i) {
                auto pj = poly_from(entity_j, motion_j);
                float ri = registry.collisionCircles.get(entity_i).radius;
                vec2 mtv;
                bool overlap = sat_polygon_circle(pj, motion_i.position, ri, mtv);
                if (overlap)
                    hit_for_damage = true;
            }


            else if ((is_bullet_i && is_enemy_j) || (is_bullet_j && is_enemy_i)) {
                float ri = radius_of(entity_i, motion_i);
                float rj = radius_of(entity_j, motion_j);
                bool overlap = circle_circle_overlap(motion_i, ri, motion_j, rj);
                if (overlap)
                    hit_for_damage = true;
            }

            // blocking/pushing
            bool hit_for_blocking = false;
            bool use_circ_i = (is_player_i && has_circ_i) || (!is_player_i && !has_col_i && has_circ_i);
            bool use_circ_j = (is_player_j && has_circ_j) || (!is_player_j && !has_col_j && has_circ_j);

            auto push_mesh_from_circle = [&](const Motion& circle_motion,
                                             float circle_radius,
                                             Entity mesh_entity,
                                             Motion& mesh_motion) {
                // compute world polygon for mesh
                vec2 mtv;
                std::vector<vec2> mesh_polygon = poly_from(mesh_entity, mesh_motion);
                bool overlaps = sat_polygon_circle(mesh_polygon,
                                                    circle_motion.position,
                                                    circle_radius,
                                                    mtv);
                if (!overlaps)
                    return false;

                // find mesh polygon centroid to find push direction
                vec2 centroid = { 0.f, 0.f };
                for (const vec2& p : mesh_polygon) {
                    centroid.x += p.x;
                    centroid.y += p.y;
                }
                float count = (float) mesh_polygon.size();
                centroid.x /= count;
                centroid.y /= count;

                // normalised direction from circle center to mesh centroid
                vec2 dir = { centroid.x - circle_motion.position.x,
                             centroid.y - circle_motion.position.y };
                float dir_len = sqrtf(dot(dir, dir));
                if (dir_len <= 0.00001f)
                    return false;
                dir.x /= dir_len;
                dir.y /= dir_len;

                // push amount
                float mtv_len = sqrtf(dot(mtv, mtv));
                mesh_motion.position.x += dir.x * mtv_len;
                mesh_motion.position.y += dir.y * mtv_len;
                return true;
            };

            if (use_circ_i && use_circ_j)
            {
                auto radius_from = [&](Entity e){
                    if (registry.collisionCircles.has(e)) return registry.collisionCircles.get(e).radius;
                    vec2 bb = get_bounding_box(registry.motions.get(e)) / 2.f;
                    return sqrtf(dot(bb, bb));
                };
                float ri = radius_from(entity_i);
                float rj = radius_from(entity_j);
                vec2 dp = motion_i.position - motion_j.position;
                float dist2 = dot(dp, dp);
                float sumr = ri + rj;
                if (dist2 < sumr*sumr)
                {
                    hit_for_blocking = true;
                    float dist = sqrt(dist2);
                    vec2 n = dist > 0.0001f ? vec2{dp.x/dist, dp.y/dist} : vec2{1.f, 0.f};
                    float overlap = sumr - dist;

                    // push enemy out
                    if (is_player_i) {
                        motion_j.position -= n * overlap;  // push enemy away
                    } else if (is_player_j) {
                        motion_i.position += n * overlap;  // push enemy away
                    } else {
                        vec2 half = {n.x * overlap * 0.5f, n.y * overlap * 0.5f};
                        // for two non-player circles push symmetrically
                        motion_i.position += half;
                        motion_j.position -= half;
                }
            }
            }
            else if (use_circ_i && !use_circ_j && has_col_j) {
                if (push_mesh_from_circle(motion_i, radius_of(entity_i, motion_i), entity_j, motion_j)) hit_for_blocking = true;
            }

            else if (use_circ_j && !use_circ_i && has_col_i) {
                if (push_mesh_from_circle(motion_j, radius_of(entity_j, motion_j), entity_i, motion_i)) hit_for_blocking = true;
            }
            else if (!use_circ_i && !use_circ_j && has_col_i && has_col_j)
            {
                std::vector<vec2> poly_i, poly_j;
                transform_polygon(motion_i, registry.colliders.get(entity_i).local_points, poly_i);
                transform_polygon(motion_j, registry.colliders.get(entity_j).local_points, poly_j);
                vec2 mtv;
                if (sat_overlap(poly_i, poly_j, mtv))
                {
                    hit_for_blocking = true;
                    vec2 half = { mtv.x*0.5f, mtv.y*0.5f };
                    motion_i.position -= half;
                    motion_j.position += half;
                }
            }
            else if ((is_player_i || is_player_j) && !hit_for_blocking)
            {
                auto radius_from = [&](Entity e){
                    if (registry.collisionCircles.has(e)) return registry.collisionCircles.get(e).radius;
                    vec2 bb = get_bounding_box(registry.motions.get(e)) / 2.f;
                    return sqrtf(dot(bb, bb));
                };
                float ri = radius_from(entity_i);

                float rj = radius_from(entity_j);
                vec2 dp = motion_i.position - motion_j.position;
                float dist2 = dot(dp, dp);
                float sumr = ri + rj;

                if (dist2 < sumr*sumr)
                {
                    hit_for_blocking = true;
                    float dist = sqrt(dist2);
                    vec2 n = dist > 0.0001f ? vec2{dp.x/dist, dp.y/dist} : vec2{1.f, 0.f};
                    float overlap = sumr - dist;
                    if (is_player_i) {
                        motion_j.position -= n * overlap; 
                    } else {
                        motion_i.position += n * overlap;
                    }
                }
            }

            if (hit_for_damage)
            {
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

}