// Ceva game client.cpp : Defines the entry point for the application.
//

#include "Ceva game client.h"

#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines (Fixes CloseWindow collision)
#pragma comment(linker, "/SUBSYSTEM:console /ENTRY:mainCRTStartup")
#include "enet/enet.h"
// 3. Clean up the conflicting macros before Raylib steps in
#undef CloseWindow
#undef ShowCursor
#undef PlaySound
#include "raylib.h"

using std::cout;
using std::string;
//network stuff
#define MAX_PLAYERS 10
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
//network stuff end
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
		pl_pos(n, std::vector<int>(2)),
		coordonates(h* s, std::vector<int>(w* s, 0)) // Fills 400x400 coordinates with 0s

	{
	}
};
const int pl_width = 16;
struct player {
	int health{};
	string name;
	int pos_x{};
	int pos_y{};
	int id{};
	bool active = true;
	player(int h, string name, int x, int y,int i) :
		health(h),
		name(name),
		pos_x(x),
		pos_y(y),
		id(i)
	{
	}
};
struct projectile {
	int radius{};
	int pos_x{};
	int pos_y{};
};
struct player_input {
	float x{};
	float y{};
	bool attack{};
	float aim_x{};
	float aim_y{};
	player_input(float x, float y, bool a) :
		x(x),
		y(y),
		attack(a)
	{
	}
};
// Helper function to return a raylib Color based on cell value
Color GetCellColor(int value) {
	switch (value) {
	case 0:  return BLACK;        // Ground
	case 1:  return BLUE;       // Special/Goal
	case 2:  return YELLOW;        // Bush
	case 3:  return BROWN;        // Wall Type A
	case 4:  return DARKGRAY;     // Outer Wall
	default: return WHITE;        // Fallback
	}
};
std::vector<Color> pl_colors = {
	Color{ 230, 57,  70,  255 }, // 0. Crimson Red (Great for Player 1 / Enemy)
	Color{ 58,  125, 203, 255 }, // 1. Electric Blue (Great for Local Player Team)
	Color{ 46,  196, 182, 255 }, // 2. Bright Teal
	Color{ 255, 159, 28,  255 }, // 3. Neon Orange
	Color{ 155, 93,  229, 255 }, // 4. Vivid Purple
	Color{ 0,   204, 102, 255 }, // 5. Emerald Green
	Color{ 255, 0,   127, 255 }, // 6. Hot Pink
	Color{ 241, 91,  181, 255 }, // 7. Bubblegum Magenta
	Color{ 255, 214, 10,  255 }, // 8. Cyber Yellow
	Color{ 0,   245, 255, 255 }
};
std::vector<Color> pl_colors_team = {
	Color{ 255, 0,  0,  255 },
	Color{ 0,  0, 255, 255 },
};
auto movement_float(bool gamepad,double d_time) {
	float joy_y = 0, joy_x = 0, aim_x = 0,aim_y=0;
	bool attack = false;
	const float deadzone = 0.1f;
	const float trigger_deadzone = -0.9f;
	if (gamepad) {
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X)) >= deadzone) {
			joy_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
		}
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y)) >= deadzone) {
			joy_y = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
		}
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X)) >= deadzone) {
			aim_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
		}
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y)) >= deadzone) {
			aim_y = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
		}
		if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER) > trigger_deadzone) {
			attack = true;
		}
	}
	if (IsKeyDown(KEY_UP)) {
		aim_y = -1;
	}
	if (IsKeyDown(KEY_DOWN)) {
		aim_y = 1;
	}
	if (IsKeyDown(KEY_LEFT)) {
		aim_x = -1;
	}
	if (IsKeyDown(KEY_RIGHT)) {
		aim_x = 1;
	}
	if (IsKeyDown(KEY_W)) {
		joy_y = -1;
	}
	if (IsKeyDown(KEY_S)) {
		joy_y = 1;
	}
	if (IsKeyDown(KEY_A)) {
		joy_x = -1;
	}
	if (IsKeyDown(KEY_D)) {
		joy_x = 1;
	}
	if (IsKeyDown(KEY_E)) {
		attack= true;
	}

	
	float length = std::sqrt(joy_x * joy_x + joy_y * joy_y);
	if (length > 1.0f) {
		joy_x /= length; // Scale X down (becomes ~0.707)
		joy_y /= length; // Scale Y down (becomes ~0.707)
	}
	length = std::sqrt(aim_x * aim_x + aim_y* aim_x);
	if (length > 1.0f) {
		aim_x /= length; // Scale X down (becomes ~0.707)
		aim_y /= length; // Scale Y down (becomes ~0.707)
	}

	player_input input(joy_x, joy_y, attack);
	input.aim_x = aim_x;
	input.aim_y = aim_y;
	return input;
}


void SendInputToServer(ENetPeer* peer, player_input input) {
	/* 2. Create the ENet Packet:
	   - Pass a pointer to your data structure
	   - Pass the size of the structure in bytes
	   - Set the flags (ENET_PACKET_FLAG_RELIABLE or 0 for unreliable UDP) */
	ENetPacket* packet = enet_packet_create(&input, sizeof(player_input), 0);

	/* 3. Send the packet over the wire:
	   - 'peer' is the server reference we got during connection
	   - '0' is the channel number (Channel 0)
	   - 'packet' is our data allocation */
	enet_peer_send(peer, 0, packet);
}


int main() {
	bool active = true;
	int my_id=-1;
	std::vector<player> players;
	std::vector<player> players_visible;
	std::vector<projectile> projectiles;
	map default_map(39, 39, 20, 6);
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
		{ 4 * default_map.scale + default_map.scale / 2, 17 * default_map.scale + default_map.scale / 2},
		{ 4 * default_map.scale + default_map.scale / 2, 19 * default_map.scale + default_map.scale / 2},
		{ 4 * default_map.scale + default_map.scale / 2, 21 * default_map.scale + default_map.scale / 2},
		{34 * default_map.scale + default_map.scale / 2, 17 * default_map.scale + default_map.scale / 2},
		{34 * default_map.scale + default_map.scale / 2, 19 * default_map.scale + default_map.scale / 2},
		{34 * default_map.scale + default_map.scale / 2, 21 * default_map.scale + default_map.scale / 2},
	};
	for (int i = 0; i < default_map.pl_pos.size(); i++) {
		player pl_temp(1000, "Player" + std::to_string(i), default_map.pl_pos[i][1], default_map.pl_pos[i][0],i);
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
	atexit(enet_deinitialize);

	// 2. Create a Client Host (Passing NULL as the address means it's an outbound client)
	ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0); // 1 outgoing connection, 2 channels
	if (client == nullptr) {
		std::cerr << "An error occurred while trying to create an ENet client host.\n";
		return 1;
	}

	// 3. Configure the destination address to target your own laptop
	ENetAddress address;
	enet_address_set_host(&address, "127.0.0.1"); // Localhost Loopback IP
	address.port = 12345;                        // Target the server's port

	// 4. Initiate the connection request
	ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
	if (peer == nullptr) {
		std::cerr << "No available peers for initiating an ENet connection.\n";
		return 1;
	}

	// 5. Connect Handshake verification
	ENetEvent event;
	// Wait up to 5000 milliseconds for the connection to succeed
	if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
		std::cout << "Connection to localhost:12345 succeeded!\n";
	}
	else {
		// Handshake failed or timed out
		enet_peer_reset(peer);
		std::cerr << "Connection to localhost:12345 failed.\n";
		enet_host_destroy(client);
		return 1;
	}



	const int screenWidth = default_map.width * default_map.scale;
	const int screenHeight = default_map.width * default_map.scale;
	InitWindow(screenWidth, screenHeight, "ceva game client");
	SetTargetFPS(144);
	while (active && !WindowShouldClose()) {
		if (IsKeyDown(KEY_X)) {
			active = false;
			continue;
		}
		player_input input = movement_float(IsGamepadAvailable(0), 1 / 30);
		SendInputToServer(peer, input);
		
		// '0' means non-blocking. Check the network card instantly and keep moving.
		while (enet_host_service(client, &event, 0) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_RECEIVE: {
				// Check if the incoming packet matches our world snapshot size
				if (event.packet->dataLength == sizeof(int)) {
					my_id = *(int*)event.packet->data;
					cout << "Server assigned me Player ID: " << my_id << std::flush;
				}
				// Check if it's the standard world broadcast packet
				else if (event.packet->dataLength == sizeof(WorldStatePacket)) {
					projectiles.clear();
					// Cast the raw byte data back into our C++ struct layout
					WorldStatePacket* incoming_world = (WorldStatePacket*)event.packet->data;
					for (int i = 0; i < incoming_world->active_player_count; i++) {
						players[incoming_world->players[i].id].id = incoming_world->players[i].id;
						players[incoming_world->players[i].id].pos_x = incoming_world->players[i].pos_x;
						players[incoming_world->players[i].id].pos_y = incoming_world->players[i].pos_y;
						players[incoming_world->players[i].id].health = incoming_world->players[i].health;
						players[incoming_world->players[i].id].active = incoming_world->players[i].active;
						
					}
						
					
					for (int i = 0; i < incoming_world->projectile_count; i++) {
						projectile pr;
						pr.pos_x = incoming_world->projectiles[i].pos_x;
						pr.pos_y = incoming_world->projectiles[i].pos_y;
						pr.radius = incoming_world->projectiles[i].radius;
						projectiles.push_back(pr);
					}
					
				}
				enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT: {
				std::cout << "The server disconnected or kicked you."<< std::endl;
				break;
			}

			default:
				break;
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);
		for (int y = 0; y < default_map.height; ++y) {
			for (int x = 0; x < default_map.width; ++x) {
				Color tileColor = GetCellColor(default_map.matrix[y][x]);
				// DrawRectangle(posX, posY, width, height, color)
				DrawRectangle(x * default_map.scale, y * default_map.scale, default_map.scale, default_map.scale, tileColor);
			}
		}
		for (int i = 0; i < players.size(); i++) {
			if (players[i].active) {
				int j;
				DrawText(TextFormat("%d", players[i].health), players[i].pos_x - pl_width / 2, players[i].pos_y - pl_width / 2 - 15, 10, GREEN);
				if (i < players.size() / 2) j = 0;
				else j = 1;
				if (my_id == i) {
					DrawRectangle(players[i].pos_x - pl_width / 2, players[i].pos_y - pl_width / 2, pl_width, pl_width, pl_colors_team[j]);
				}

				else {
					DrawRectangle(players[i].pos_x - pl_width / 2, players[i].pos_y - pl_width / 2, pl_width, pl_width, pl_colors_team[j]);
				}
			}
		}
		
		for (int i = 0; i < projectiles.size(); i++) {
			DrawCircle(projectiles[i].pos_x, projectiles[i].pos_y, projectiles[i].radius, Color{ 155, 93,  229, 255 });
		}
		
		DrawFPS(10, 10);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
