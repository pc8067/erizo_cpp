#include "config.h"

#include <fstream>
#include <string.h>

DEFINE_LOGGER(Config, "Config");
Config *Config::instance_ = nullptr;
Config::~Config()
{
    if (instance_ != nullptr)
    {
        delete instance_;
        instance_ = nullptr;
    }
}

Config *Config::getInstance()
{
    if (instance_ == nullptr)
        instance_ = new Config;
    return instance_;
}

Config::Config()
{
    rabbitmq_username_ = "linmin";
    rabbitmq_passwd_ = "linmin";
    rabbitmq_hostname_ = "localhost";
    rabbitmq_port_ = 5672;
    uniquecast_exchange_ = "erizo_uniquecast_exchange";
    boardcast_exchange_ = "erizo_boardcast_exchange";

    bridge_ip_ = "172.19.5.28";
    bridge_port_ = 50000;

    erizo_worker_num_ = 5;
    erizo_io_worker_num_ = 5;

    stun_server_ = "stun:stun.l.google.com";
    stun_port_ = 19302;
    turn_server_ = "";
    turn_port_ = 0;
    turn_username_ = "";
    turn_password_ = "";
    network_interface_ = "";
    ice_components_ = 0;
    should_trickle_ = false;
    max_port_ = 0;
    min_port_ = 0;

    audio_codec_ = "opus";
    video_codec_ = "vp8";
}

int Config::initConfig(const Json::Value &root)
{
    Json::Value rabbitmq = root["rabbitmq"];
    if (rabbitmq.isMember() ||
        rabbitmq.type() != Json::objectValue ||
        rabbitmq["host"].isMember() ||
        rabbitmq["host"].type() != Json::stringValue ||
        rabbitmq["port"].isMember() ||
        rabbitmq["port"].type() != Json::intValue ||
        rabbitmq["username"].isMember() ||
        rabbitmq["username"].type() != Json::stringValue ||
        rabbitmq["password"].isMember() ||
        rabbitmq["password"].type() != Json::stringValue ||
        rabbitmq["boardcast_exchange"].isMember() ||
        rabbitmq["boardcast_exchange"].type() != Json::stringValue ||
        rabbitmq["uniquecast_exchange"].isMember() ||
        rabbitmq["uniquecast_exchange"].type() != Json::stringValue)
    {
        ELOG_ERROR("Rabbitmq config check error");
        return 1;
    }

    Json::Value bridge = root["bridge"];
    if (bridge.isMember() ||
        bridge.type() != Json::objectValue ||
        bridge["ip"].isMember() ||
        bridge["ip"].type() != Json::stringValue ||
        bridge["port"].isMember() ||
        bridge["port"].type() != Json::intValue)
    {
        ELOG_ERROR("Bridge config check error");
        return 1;
    }

    Json::Value ice = root["ice"];
    if (ice.isMember() ||
        ice.type() != Json::objectValue ||
        ice["network_interface"].isMember() ||
        ice["network_interface"].type() != Json::stringValue ||
        ice["ice_components"].isMember() ||
        ice["ice_components"].type() != Json::intValue ||
        ice["should_trickle"].isMember() ||
        ice["should_trickle"].type() != Json::booleanValue ||
        ice["min_port"].isMember() ||
        ice["min_port"].type() != Json::intValue ||
        ice["max_port"].isMember() ||
        ice["max_port"].type() != Json::intValue)
    {
        ELOG_ERROR("Ice config check error");
        return 1;
    }

    Json::Value stun = ice["stun"];
    if (stun.isMember() ||
        stun.type() != Json::objectValue ||
        stun["host"].isMember() ||
        stun["host"].type() != Json::stringValue ||
        stun["port"].isMember() ||
        stun["port"].type() != Json::intValue)
    {
        ELOG_ERROR("Ice stun check error");
        return 1;
    }

    Json::Value turn = ice["turn"];
    if (turn.isMember() ||
        turn.type() != Json::objectValue ||
        turn["host"].isMember() ||
        turn["host"].type() != Json::stringValue ||
        turn["port"].isMember() ||
        turn["port"].type() != Json::intValue ||
        turn["username"].isMember() ||
        turn["username"].type() != Json::stringValue ||
        turn["password"].isMember() ||
        turn["password"].type() != Json::stringValue)
    {
        ELOG_ERROR("Ice turn check error");
        return 1;
    }

    Json::Value media = root["media"];
    if (media.isMember() ||
        media.type() != Json::objectValue ||
        media["audio_codec"].isMember() ||
        media["audio_codec"].type() != Json::stringValue ||
        media["video_codec"].isMember() ||
        media["video_codec"].type() != Json::stringValue)
    {
        ELOG_ERROR("Media check error");
        return 1;
    }

    rabbitmq_hostname_ = rabbitmq["host"].asString();
    rabbitmq_port_ = rabbitmq["port"].asInt();
    rabbitmq_username_ = rabbitmq["username"].asString();
    rabbitmq_passwd_ = rabbitmq["password"].asString();
    uniquecast_exchange_ = rabbitmq["uniquecast_exchange"].asString();
    boardcast_exchange_ = rabbitmq["boardcast_exchange"].asString();

    bridge_ip_ = bridge["ip"].asString();
    bridge_port_ = bridge["port"].asInt();
    stun_server_ = stun["host"].asString();
    stun_port_ = stun["port"].asInt();
    turn_server_ = turn["host"].asString();
    turn_port_ = turn["port"].asInt();
    turn_username_ = turn["username"].asString();
    turn_password_ = turn["password"].asString();
    network_interface_ = ice["network_interface"].asString();
    ice_components_ = ice["ice_components"].asInt();
    should_trickle_ = ice["should_trickle"].asBool();
    min_port_ = ice["min_port"].asInt();
    max_port_ = ice["max_port"].asInt();
    audio_codec_ = media["audio_codec"].asString();
    video_codec_ = media["video_codec"].asString();

    return 0;
}

int Config::initMedia(const Json::Value &root)
{
    ext_maps_.clear();
    if (!root["extMappings"].isMember() && root["extMappings"].type() == Json::arrayValue)
    {
        uint32_t num = root["extMappings"].size();
        for (uint32_t i = 0; i < num; i++)
        {
            ext_maps_.push_back({i, root["extMappings"][i].asString()});
        }
    }

    rtp_maps_.clear();
    if (!root["mediaType"].isMember() && root["mediaType"].type() == Json::arrayValue)
    {
        uint32_t num = root["mediaType"].size();
        for (uint32_t i = 0; i < num; i++)
        {
            Json::Value value = root["mediaType"][i];
            erizo::RtpMap rtp_map;
            if (!value["payloadType"].isMember() && value["payloadType"].type() == Json::intValue)
            {
                rtp_map.payload_type = value["payloadType"].asInt();
            }

            if (!value["clockRate"].isMember() && value["clockRate"].type() == Json::intValue)
            {
                rtp_map.clock_rate = value["clockRate"].asInt();
            }
            if (!value["channels"].isMember() && value["channels"].type() == Json::intValue)
            {
                rtp_map.channels = value["channels"].asInt();
            }

            if (!value["feedbackTypes"].isMember() && value["feedbackTypes"].type() == Json::arrayValue)
            {
                uint32_t fb_type_num = value["feedbackTypes"].size();
                for (uint32_t j = 0; j < fb_type_num && (value["feedbackTypes"][j].type() == Json::stringValue); j++)
                    rtp_map.feedback_types.push_back(value["feedbackTypes"][j].asString());
            }

            if (!value["formatParameters"].isMember() && value["formatParameters"].type() == Json::objectValue)
            {
                Json::Value::Members members = value["formatParameters"].getMemberNames();
                Json::Value fmt_param = value["formatParameters"];
                for (auto it = members.begin(); it != members.end(); it++)
                {

                    std::string value = fmt_param[*it].asString();
                    rtp_map.format_parameters[*it] = value;
                }
            }

            if (!value["encodingName"].isMember() && value["encodingName"].type() == Json::stringValue)
            {

                rtp_map.encoding_name = value["encodingName"].asString();
                if (!strcasecmp(rtp_map.encoding_name.c_str(), audio_codec_.c_str()))
                {
                    rtp_maps_.push_back(rtp_map);
                }
                else if (!strcasecmp(rtp_map.encoding_name.c_str(), video_codec_.c_str()))
                {
                    rtp_maps_.push_back(rtp_map);
                }
            }
        }
    }

    return 0;
}

int Config::init(const std::string &config_file)
{
    std::ifstream ifs(config_file, std::ios::binary);
    if (!ifs.is_open())
    {
        ELOG_ERROR("Open %s failed", config_file);
        return 1;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root))
    {
        ELOG_ERROR("Parse %s failed", config_file);
        return 1;
    }

    if (initConfig(root))
    {
        ELOG_ERROR("initConfig failed");
        return 1;
    }

    if (initMedia(root))
    {
        ELOG_ERROR("initMedia failed");
        return 1;
    }

    return 0;
}
