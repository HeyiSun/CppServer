#include "CppServer/config.h"
#include "CppServer/log.h"
#include <yaml-cpp/yaml.h>

CppServer::ConfigVar<int>::ptr g_int_value_config = 
    CppServer::Config::Lookup("system.port", (int)8080, "system port");

CppServer::ConfigVar<float>::ptr g_float_value_config = 
    CppServer::Config::Lookup("system.value", (float)10.52, "system value");

CppServer::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
    CppServer::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vector");

CppServer::ConfigVar<std::list<int>>::ptr g_int_list_value_config = 
    CppServer::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");

CppServer::ConfigVar<std::set<int>>::ptr g_int_set_value_config = 
    CppServer::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");

CppServer::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config = 
    CppServer::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2}, "system int uset");

CppServer::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config = 
    CppServer::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");
CppServer::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config = 
    CppServer::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}}, "system str int umap");

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

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto&& i : v) { \
            CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto&& i : v) { \
            CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << #prefix " " #name ": {" \
                        << i.first << " - " << i.second << "}"; \
        } \
        CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/heyisun/Documents/playground/cpp/CppServer/bin/conf/log.yml");
    CppServer::Config::LoadFromYaml(root);

    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "after:" << g_float_value_config->toString();


    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

class Person {
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = false;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name 
           << " age=" << m_age
           << " sex=" << m_sex
           << "}";
        return ss.str();
    }
};


namespace CppServer {
    template <>
    class LexicalCast<std::string, Person> {
    public:
        Person operator() (const std::string& v) {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age  = node["age"].as<int>();
            p.m_sex  = node["sex"].as<bool>();
            return p;
        }
    };
    template <>
    class LexicalCast<Person, std::string> {
    public:
        std::string operator() (const Person& p) {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}


CppServer::ConfigVar<Person>::ptr g_person = 
    CppServer::Config::Lookup("class.person", Person(), "system person");

CppServer::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = 
    CppServer::Config::Lookup("class.map", std::map<std::string, Person>(), "system person map");

CppServer::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map = 
    CppServer::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system (person vector) map");

void test_class() {
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    {\
        auto m = g_person_map->getValue(); \
        for (auto& i : m) { \
            CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << prefix << i.first << " - " << i.second.toString(); \
        } \
        CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << prefix << ": size=" << m.size(); \
    }
    XX_PM(g_person_map, "class.map before ");
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "before " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/heyisun/Documents/playground/cpp/CppServer/bin/conf/log.yml");
    CppServer::Config::LoadFromYaml(root);

    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after ");
    CPPSERVER_LOG_INFO(CPPSERVER_LOG_ROOT()) << "after " << g_person_vec_map->toString();
}


int main(int argc, char** argv) {

    // test_yaml();
    // test_config();
    test_class();
    return 0;
}
