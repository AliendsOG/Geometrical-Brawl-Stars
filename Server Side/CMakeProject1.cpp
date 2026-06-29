#include "CMakeProject1.h"
//#define NOGDI             // All GDI defines and routines
//#define NOUSER            // All USER defines and routines (Fixes CloseWindow collision)

#include "enet/enet.h"
// 3. Clean up the conflicting macros before Raylib steps in
//#undef CloseWindow
//#undef ShowCursor
//#undef PlaySound
//#include "raylib.h"
using std::cout;
using std::string;


#define MAX_PLAYERS 10
#define MAX_PROJECTILES 100

struct PlayerNetworkState {
	uintptr_t id{};
	int pos_x{};
	int pos_y{};
	int health{};
	bool active{};
};
struct projectile_network {
	int pos_x{};
	int pos_y{};
	int radius{};
};

// This is the full package sent over the network
struct WorldStatePacket {
	int projectile_count{};
	int active_player_count{};
	PlayerNetworkState players[MAX_PLAYERS];
	projectile_network projectiles[MAX_PROJECTILES];
};
struct map {
	int height{};
	int width{};
	int scale{};
	int pl_nr{};
	std::vector<std::vector<int>> matrix;
	std::vector<std::vector<int>> coordonates;
	std::vector<std::vector<int>> pl_pos;
	map(int h, int w, int s, int n) :
		height(h),
		width(w),
		scale(s),
		pl_nr(n),
		pl_pos(n,std::vector<int>(2)),
		coordonates(h*s, std::vector<int>(w*s, 0)) // Fills 400x400 coordinates with 0s

	{}
};
const int pl_width = 16;
struct player {
	int id{};
	int health{};
	string name;
	int ammo{};
	float reload_time{};
	long long reload_time_start{};
	long long reload_time_now{};
	int range{};
	int ammo_r{};
	int ammo_speed{};
	int ammo_damage{};
	int pos_x{};
	int pos_y{};
	int new_x{};
	int new_y{};
	float pos_f_x{};
	float pos_f_y{};
	float joy_x{};
	float joy_y{};
	int speed{};
	float aim_x{};
	float aim_y{};
	bool attack = false;
	bool active = true;
	player(int h, string name, int x, int y, int range, int s, int a_r, int a_speed,int id, int a_d) :
		health(h),
		id(id),
		name(name),
		pos_x(x),
		pos_y(y),
		new_x(x),
		new_y(y),
		pos_f_x(static_cast<int>(x)),
		pos_f_y(static_cast<int>(y)),
		range(range),
		speed(s),
		ammo_r(a_r),
		ammo_speed(a_speed),
		ammo_damage(a_d)
	{}
};
struct projectile {
	int id{};
	int pr_id{};
	int radius{};
	int speed{};
	int pos_x{};
	int pos_y{};
	int new_x{};
	int new_y{};
	int range{};
	int damage{};
	float distance{};
	float pos_f_x{};
	float pos_f_y{};
	float aim_x{};
	float aim_y{};
	projectile(int r,int s,int range, int damage):
		radius(r),
		speed(s),
		range(range),
		damage(damage)
	{}
};

struct player_input {
	float x{};
	float y{};
	bool attack{};
	float aim_x{};
	float aim_y{};
};

// Helper function to return a raylib Color based on cell value
//Color GetCellColor(int value) {
//	switch (value) {
//	case 0:  return BLACK;        // Ground
//	case 1:  return BLUE;       // Special/Goal
//	case 2:  return YELLOW;        // Bush
//	case 3:  return BROWN;        // Wall Type A
//	case 4:  return DARKGRAY;     // Outer Wall
//	default: return WHITE;        // Fallback
//	}
//};
//std::vector<Color> pl_colors = {
//	Color{ 230, 57,  70,  255 }, // 0. Crimson Red (Great for Player 1 / Enemy)
//	Color{ 58,  125, 203, 255 }, // 1. Electric Blue (Great for Local Player Team)
//	Color{ 46,  196, 182, 255 }, // 2. Bright Teal
//	Color{ 255, 159, 28,  255 }, // 3. Neon Orange
//	Color{ 155, 93,  229, 255 }, // 4. Vivid Purple
//	Color{ 0,   204, 102, 255 }, // 5. Emerald Green
//	Color{ 255, 0,   127, 255 }, // 6. Hot Pink
//	Color{ 241, 91,  181, 255 }, // 7. Bubblegum Magenta
//	Color{ 255, 214, 10,  255 }, // 8. Cyber Yellow
//	Color{ 0,   245, 255, 255 }
//};
//std::vector<Color> pl_colors_team = {
//	Color{ 255, 0,  0,  255 }, 
//	Color{ 0,  0, 255, 255 }, 
//};

bool collision(map& map, int& new_x, int& new_y) {
	int br_x = new_x + pl_width/2-1; //buttom-right corner
	int br_y = new_y + pl_width/2-1; 
	int tl_x = new_x - pl_width/2; //top-left corner
	int tl_y = new_y - pl_width/2;
	int tr_x = new_x + pl_width/2-1; //top-right corner
	int tr_y = new_y - pl_width/2;
	int bl_x = new_x - pl_width/2; //buttom-left corner
	int bl_y = new_y + pl_width/2-1;
	if (tl_x < 0 || br_x >= map.width*map.scale || tl_y < 0 || br_y >= map.height*map.scale) return false;
	else{
		if (((map.coordonates[br_y][br_x] == 0) || (map.coordonates[br_y][br_x] == 2) || (map.coordonates[br_y][br_x] == -1)) &&
			((map.coordonates[tl_y][tl_x] == 0) || (map.coordonates[tl_y][tl_x] == 2) || (map.coordonates[tl_y][tl_x] == -1)) &&
			((map.coordonates[bl_y][bl_x] == 0) || (map.coordonates[bl_y][bl_x] == 2) || (map.coordonates[bl_y][bl_x] == -1)) &&
			((map.coordonates[tr_y][tr_x] == 0) || (map.coordonates[tr_y][tr_x] == 2) || (map.coordonates[tr_y][tr_x] == -1))) {
			return true;
		}
		else {
			return false;
		} 
	}
}
bool aim(map& map, std::vector<player>& players, projectile& pr, float d_time) {
	float length = std::sqrt(pr.aim_x * pr.aim_x + pr.aim_y * pr.aim_y);
	float distance_min = pl_width / 2 + pr.radius;
	if (length > 1.0f) {
		pr.aim_x /= length; // Scale X down (becomes ~0.707) if needed, cause it is a little redundant
		pr.aim_y /= length; // Scale Y down (becomes ~0.707) if needed, cause it is a little redundant
	}
	if((pr.aim_x==0)&&(pr.aim_y==0)) return false;
	pr.pos_f_x += pr.aim_x * pr.speed * d_time;
	pr.pos_f_y += pr.aim_y * pr.speed * d_time;
	pr.new_x = static_cast<int>(pr.pos_f_x);
	pr.new_y = static_cast<int>(pr.pos_f_y);
	if (pr.distance >= pr.range) {
		return false;
	}
	int* tile = &map.coordonates[pr.new_y][pr.new_x];
	if ((*tile == 3) || (*tile == 4)) {
		return false;
	}
	pr.distance += static_cast<int>(std::sqrt(pow(pr.new_x - pr.pos_x, 2) + pow(pr.new_y - pr.pos_y, 2)));
	pr.pos_x = pr.new_x;
	pr.pos_y = pr.new_y;
	for (int i = 0; i < players.size(); i++) {
		if ((pr.id != i)&&players[i].active) {
			if ((abs(players[i].pos_x - pr.pos_x) <= distance_min) &&(abs(players[i].pos_y - pr.pos_y) <= distance_min)) {
				players[i].health -= pr.damage;
				return false;
			}
		}
	}
	return true;
}

void movement_float(map& map, player& pl, double d_time) {
	float length = std::sqrt(pl.joy_x * pl.joy_x + pl.joy_y * pl.joy_y);
	if (length > 1.0f) {
		pl.joy_x /= length; // Scale X down (becomes ~0.707) if needed, cause it is a little redundant
		pl.joy_y /= length; // Scale Y down (becomes ~0.707) if needed, cause it is a little redundant
	}
	pl.pos_f_x += pl.joy_x * pl.speed * d_time;
	pl.pos_f_y += pl.joy_y * pl.speed * d_time;
	pl.new_x = static_cast<int>(pl.pos_f_x);
	pl.new_y = static_cast<int>(pl.pos_f_y);

	if ((pl.new_x != pl.pos_x) && (collision(map, pl.new_x, pl.pos_y))) {
		pl.pos_x = pl.new_x;
	}
	else {

		pl.pos_f_x = static_cast<float>(pl.pos_x);
	}
	if ((pl.new_y != pl.pos_y) && (collision(map, pl.pos_x, pl.new_y))) {
		pl.pos_y = pl.new_y;	
	}
	else {
		pl.pos_f_y = static_cast<float>(pl.pos_y);
	}	
}



int main() {
	bool active = true;
	std::vector<player> players;
	std::vector<projectile> projectiles;
	map default_map(39, 39,20,6);
	default_map.matrix = {
		{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, // 1
		{4,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,4}, // 2
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 3
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 4
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 5
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 6
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 7
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 8
		{4,2,2,2,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,2,2,2,4}, // 9
		{4,0,0,0,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,0,0,0,4}, // 10
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 11
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 12
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 13
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 14
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 15
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 16
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 17
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 18
		{4,0,0,4,4,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,4,4,0,0,4}, // 19
		{4,0,0,4,4,0,0,0,0,0,0,0,0,0,0,3,3,0,0,4,0,0,3,3,0,0,0,0,0,0,0,0,0,0,4,4,0,0,4}, // 20 
		{4,0,0,4,4,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,4,4,0,0,4}, // 21
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 22
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 23
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 24
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 25
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 26
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 27
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 28
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 29
		{4,0,0,0,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,0,0,0,4}, // 30
		{4,2,2,2,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,2,2,2,4}, // 31
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 32
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 33
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 34
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 35
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 36
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 37
		{4,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,4}, // 38
		{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}  // 39
	};
	default_map.pl_pos = {
		{ 4 * default_map.scale + default_map.scale/2, 17 * default_map.scale + default_map.scale/2},
		{ 4 * default_map.scale + default_map.scale/2, 19 * default_map.scale + default_map.scale/2},
		{ 4 * default_map.scale + default_map.scale/2, 21 * default_map.scale + default_map.scale/2},
		{34 * default_map.scale + default_map.scale/2, 17 * default_map.scale + default_map.scale/2},
		{34 * default_map.scale + default_map.scale/2, 19 * default_map.scale + default_map.scale/2},
		{34 * default_map.scale + default_map.scale/2, 21 * default_map.scale + default_map.scale/2},
	};
	for (int i = 0; i < default_map.pl_pos.size(); i++) {
		player pl_temp(1000, "Player" + std::to_string(i), default_map.pl_pos[i][1], default_map.pl_pos[i][0], 100, 100, 4, 150,i, 100);
		pl_temp.reload_time = 1;
		pl_temp.ammo = 3;
		default_map.coordonates[pl_temp.pos_y][pl_temp.pos_x] = -i-1;
		pl_temp.active = true;
		players.push_back(pl_temp);
	}
	for (int i = 0; i < default_map.height; i++) {
		for (int j = 0; j < default_map.width; j++) {
			for (int h = 0; h < default_map.scale; h++) {
				for (int w = 0; w < default_map.scale; w++) {
					default_map.coordonates[i * default_map.scale + h][j * default_map.scale + w] = default_map.matrix[i][j];
				}
			}
			
		}
	}
	if (enet_initialize() != 0) {
		std::cerr << "An error occurred while initializing ENet.\n";
		return 1;
	}
	// Schedule deinitialization when main() exits
	atexit(enet_deinitialize);

	// 2. Set up the address configuration
	ENetAddress address;
	address.host = ENET_HOST_ANY; // Listen to incoming connections on your laptop
	address.port = 12345;         // The dedicated port for your game

	// 3. Create the Server Host
	ENetHost* server = enet_host_create(&address, 32, 2, 0, 0); // 32 max players, 2 channels
	if (server == nullptr) {
		std::cerr << "An error occurred while trying to create an ENet server host.\n";
		return 1;
	}

	cout << "Server started on port 12345. Waiting for clients...\n";

	// 4. The Server Event Loop
	ENetEvent event;
	const double tick_rate = 30;
	const double tick_time = 1 / tick_rate;
	double accumulated_time = 0;
	auto last_time = std::chrono::high_resolution_clock::now();
	int players_connected = 0;
	int projectile_ids = 0;
	while (active) {
		while (enet_host_service(server, &event, 0) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				cout << "A new client connected from "
					<< (*event.peer).address.host << ":"
					<< event.peer->address.port << "\n";
				// You can store player info inside the peer's custom pointer slot
				event.peer->data = (void*)(uintptr_t)players_connected;	
				ENetPacket* packet = enet_packet_create(&players_connected, sizeof(int), 0);
				enet_peer_send(event.peer, 0, packet);
				players_connected++;
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
				//cout << "Packet received from " << (uintptr_t)event.peer->data
				//	<< " | Size: " << event.packet->dataLength << " bytes"<< std::endl;
				if (event.packet->dataLength == sizeof(player_input)) {
					// Cast the raw byte packet->data pointer back into our clean C++ struct
					player_input* received_input = (player_input*)event.packet->data;
					uintptr_t target_id = (uintptr_t)event.peer->data;
					for (auto& pl : players) {
						if (pl.id == target_id) {
							pl.joy_x = received_input->x;
							pl.joy_y = received_input->y;
							pl.attack = received_input->attack;
							if (received_input->attack) {
								pl.aim_x =received_input->aim_x;
								pl.aim_y = received_input->aim_y;
							}
							else {
								pl.aim_x = 0;
								pl.aim_y = 0;
							}
							break;
						}
					}
					// Now you can update your server-side simulation!
					//cout << "Player "<< (uintptr_t)event.peer->data<<" moved to X : " << received_input->x
					//	<< " Y: " << received_input->y << std::endl;

				}
				else {
					std::cerr << "Received a malformed packet of unexpected size.\n";
				}
				// Clean up the packet memory after reading it!
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT: {
				cout << (uintptr_t)event.peer->data << " disconnected.\n";
				uintptr_t disconnecting_id = (uintptr_t)event.peer->data;

				// Find and remove the player from the vector
				for (auto it = players.begin(); it != players.end(); ++it) {
					if (it->id == disconnecting_id) {
						players.erase(it);
						break; // Found them, stop looking
					}
				}
				event.peer->data = nullptr;
				break;
			}

			case ENET_EVENT_TYPE_NONE:
				break;
			}
		}
		// 3. Calculate Delta Time manually using <chrono>
		auto current_time = std::chrono::high_resolution_clock::now();
		// Find the difference in microseconds, then convert it to a fraction of a second (double)
		std::chrono::duration<double> elapsed = current_time - last_time;
		last_time = current_time;
		// Add the real-world time passed into our bank
		accumulated_time += elapsed.count();
		WorldStatePacket world_snapshot = { 0 };
		while (accumulated_time >= tick_time){	
			for (int i = 0; i < players.size(); i++) {
				world_snapshot.players[i].id = players[i].id;
				if (players[i].active) {
					movement_float(default_map, players[i], tick_time);
					if (players[i].attack) {
						projectile pr(players[i].ammo_r, players[i].ammo_speed, players[i].range, players[i].ammo_damage);
						pr.pos_x = players[i].pos_x;
						pr.pos_y = players[i].pos_y;
						pr.pos_f_x = static_cast<float>(players[i].pos_x);
						pr.pos_f_y = static_cast<float>(players[i].pos_y);
						pr.id = players[i].id;
						pr.pr_id = projectile_ids;
						pr.aim_x = players[i].aim_x;
						pr.aim_y = players[i].aim_y;
						pr.distance = 0;
						projectile_ids++;
						projectiles.push_back(pr);
						players[i].attack = false;
					}
					world_snapshot.players[i].pos_x = players[i].pos_x;
					world_snapshot.players[i].pos_y = players[i].pos_y;
					world_snapshot.players[i].health = players[i].health;
					if (players[i].health <= 0) {
						players[i].active = false;
						world_snapshot.players[i].active = false;
					}
					else {
						players[i].active = true;
						world_snapshot.players[i].active = true;
					}
				}
				else {
					world_snapshot.players[i].active = false;
					world_snapshot.players[i].health = 0;
				}
			}
			int i = 0;
			auto it = projectiles.begin();
			while (it != projectiles.end()) {

				// 1. Run the aim logic and capture if it's alive or dead
				bool is_alive = aim(default_map, players, *it, tick_time);

				if (!is_alive) {
					// 2. If false, erase cleanly! 
					// erase() automatically returns the next valid iterator position.
					it = projectiles.erase(it);
				}
				else {
					// 3. If true, copy the updated positions to your network packet snapshot
					world_snapshot.projectiles[i].pos_x = it->pos_x;
					world_snapshot.projectiles[i].pos_y = it->pos_y;
					world_snapshot.projectiles[i].radius = it->radius;
					i++;

					++it; // Manually advance to the next element safely
				}
			}
			world_snapshot.active_player_count = players.size();
			world_snapshot.projectile_count = projectiles.size();

			// 2. Create the ENet packet containing this snapshot
			ENetPacket* state_packet = enet_packet_create(&world_snapshot,
				sizeof(WorldStatePacket),
				0);
			enet_host_broadcast(server, 0, state_packet);
			accumulated_time -= tick_time;
		}
		double time_left = tick_time - accumulated_time;
		// 1. SAFE SLEEP: If we have plenty of time (more than 5ms), give the CPU a real break
		if (time_left > 0.005) {
			// Convert our seconds (double) into integer milliseconds for the sleep function
			long long sleep_us = static_cast<long long>((time_left - 0.002) * 1000000);

			if (sleep_us > 0) {
				std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
			}
		}
	}
	enet_host_destroy(server);
	return 0;
}
	