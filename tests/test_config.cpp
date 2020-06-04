#include "CppServer/config.h"
#include "CppServer/log.h"
#include <yaml-cpp/yaml.h>

CppServer::ConfigVar<int>::ptr g_int_value_config = 
    CppServer::Config::Lookup("system.port", (int)8080, "system port");
CppServer::ConfigVar<float>::ptr g_float_value_config = 
    CppServer::Config::Lookup("system.value", (float)10.52, "system value");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << "-" << node.Type() <<" Scalar" << "-" << level;
    } else if (node.IsNull()) {
        CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL-" << node.Type() << "-" << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin();
                 it != node.end(); ++it) {
            CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << "-" << it->second.Type() << " Map" <<"-" << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << i << "-" << node[i].Type() << " Seq" << "-" << level;
            print_yaml(node[i], level + 1);
        }
        
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/heyisun/Documents/playground/cpp/CppServer/bin/conf/log.yml");
    print_yaml(root, 0);
    // CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << root;
}

void test_config() {
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "before:" << g_int_value_config->getValue();
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "before:" << g_float_value_config->toString();

    YAML::Node root = YAML::LoadFile("/home/heyisun/Documents/playground/cpp/CppServer/bin/conf/log.yml");
    CppServer::Config::LoadFromYaml(root);

    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "after:" << g_float_value_config->toString();
}


int main(int argc, char** argv) {

    // test_yaml();
    test_config();
    return 0;
}
