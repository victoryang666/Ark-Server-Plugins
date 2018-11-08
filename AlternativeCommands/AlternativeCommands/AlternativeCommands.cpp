#include <API/ARK/Ark.h>

#include <fstream>

#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json config;

struct Command
{
	Command(FString command, FString exec)
		: command(std::move(command)),
		  exec(std::move(exec))
	{
	}

	FString command;
	FString exec;
};

TArray<std::unique_ptr<Command>> commands;

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/AlternativeCommands/config.json";
	std::ifstream file{config_path};
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();

	commands.Empty();

	auto commands_map = config["Commands"];
	for (auto iter = commands_map.begin(); iter != commands_map.end(); ++iter)
	{
		const FString key = ArkApi::Tools::Utf8Decode(iter.key()).c_str();
		FString command = ArkApi::Tools::Utf8Decode(iter.value()["Command"]).c_str();

		commands.Add(std::make_unique<Command>(key, command));
	}
}

void Load()
{
	Log::Get().Init("AlternativeCommands");

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	for (const auto& command : commands)
	{
		ArkApi::GetCommands().AddChatCommand(command->command,
		                                     [&command](AShooterPlayerController* player_controller, FString* message,
		                                                EChatSendMode::Type)
		                                     {
			                                     TArray<FString> parsed;
			                                     message->ParseIntoArray(parsed, L" ", true);

			                                     if (!message->RemoveFromStart(parsed[0]))
				                                     return;

			                                     FString cmd = command->exec + *message;
			                                     player_controller->ServerSendChatMessage_Implementation(
				                                     &cmd, EChatSendMode::LocalChat);
		                                     });
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
