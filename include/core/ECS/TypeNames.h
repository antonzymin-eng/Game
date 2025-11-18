// ============================================================================
// TypeNames.h - Clean type name utilities for better debugging
// Location: include/core/ECS/TypeNames.h
// Created: November 18, 2025
// ============================================================================

#pragma once

#include <string>
#include <typeinfo>
#include <cstring>

#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>
#endif

namespace core::ecs {

    /**
     * @brief Get a clean, human-readable type name for debugging
     *
     * Demangles C++ type names on GCC/Clang and provides clean output.
     * Falls back to typeid().name() on other compilers.
     *
     * @tparam T The type to get the name for
     * @return Clean string representation of the type name
     *
     * Examples:
     *   GetTypeName<int>() -> "int"
     *   GetTypeName<MyComponent>() -> "MyComponent" (not "N4game9component11MyComponentE")
     */
    template<typename T>
    inline std::string GetTypeName() {
        const char* mangled_name = typeid(T).name();

#ifdef __GNUG__
        // GCC/Clang: Demangle using __cxa_demangle
        int status = 0;
        std::unique_ptr<char, void(*)(void*)> demangled(
            abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status),
            std::free
        );

        if (status == 0 && demangled) {
            std::string result(demangled.get());

            // Remove common namespace prefixes for cleaner output
            const char* prefixes[] = {"game::core::", "core::ecs::", "std::", "game::"};
            for (const char* prefix : prefixes) {
                size_t prefix_len = std::strlen(prefix);
                if (result.compare(0, prefix_len, prefix) == 0) {
                    result = result.substr(prefix_len);
                }
            }

            return result;
        }
#endif

        // Fallback or MSVC: Use mangled name
        return std::string(mangled_name);
    }

    /**
     * @brief Get a clean type name from an object instance
     *
     * @tparam T The type of the object
     * @param obj The object instance
     * @return Clean string representation of the type name
     */
    template<typename T>
    inline std::string GetTypeNameOf(const T& obj) {
        return GetTypeName<T>();
    }

    /**
     * @brief Type name registry for custom component names
     *
     * Allows components to register custom display names for debugging.
     * This is optional but recommended for better debugging output.
     */
    class TypeNameRegistry {
    private:
        std::unordered_map<std::type_index, std::string> m_custom_names;
        mutable std::shared_mutex m_mutex;

        TypeNameRegistry() = default;

    public:
        static TypeNameRegistry& Instance() {
            static TypeNameRegistry instance;
            return instance;
        }

        /**
         * @brief Register a custom display name for a type
         *
         * @tparam T The type to register
         * @param custom_name Human-readable name for debugging
         *
         * Example:
         *   TypeNameRegistry::Instance().Register<PopulationComponent>("Population");
         */
        template<typename T>
        void Register(const std::string& custom_name) {
            std::unique_lock lock(m_mutex);
            m_custom_names[std::type_index(typeid(T))] = custom_name;
        }

        /**
         * @brief Get the registered custom name for a type
         *
         * @tparam T The type to look up
         * @return Custom name if registered, otherwise demangled type name
         */
        template<typename T>
        std::string GetName() const {
            std::shared_lock lock(m_mutex);
            auto it = m_custom_names.find(std::type_index(typeid(T)));
            if (it != m_custom_names.end()) {
                return it->second;
            }
            return GetTypeName<T>();
        }

        /**
         * @brief Clear all registered custom names
         */
        void Clear() {
            std::unique_lock lock(m_mutex);
            m_custom_names.clear();
        }
    };

    /**
     * @brief Helper macro to auto-register component names
     *
     * Usage in component header:
     *   REGISTER_COMPONENT_NAME(PopulationComponent, "Population")
     */
    #define REGISTER_COMPONENT_NAME(TypeName, DisplayName) \
        namespace { \
            struct TypeName##_NameRegistrar { \
                TypeName##_NameRegistrar() { \
                    core::ecs::TypeNameRegistry::Instance().Register<TypeName>(DisplayName); \
                } \
            }; \
            static TypeName##_NameRegistrar g_##TypeName##_name_registrar; \
        }

} // namespace core::ecs
