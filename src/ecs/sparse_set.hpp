#pragma once
#include "src/core/types.hpp"
#include "src/core/assert.hpp"
#include "src/core/platform.hpp"
#include "src/ecs/entity.hpp"

#include <memory>
#include <span>
#include <type_traits>
#include <utility>

namespace tessera::ecs {
    template <typename T>
    requires std::is_move_constructible_v<T>
    class SparseSet {
        private: 
            EntityID* dense_entities_; // tells what entity ID is sitting here
            T* dense_components_; // the actual data 
            u32** sparse_pages_; // big array holding dense index of where data lives
            usize size_;
            usize capacity_;
            usize page_count_;

            // helper functions stuff
            [[nodiscard]] static auto entity_index(EntityID entity) noexcept -> u32 {
                return static_cast<u32>(entity) & 0xFFFFF; // 20-bit index
            }
        
            [[nodiscard]] auto sparse_ptr(u32 index) const noexcept -> u32* {
                const usize page = index / ENTITIES_PER_PAGE;
                if (page >= page_count_ || sparse_pages_[page] == nullptr) {
                    return nullptr;
                }
                return &sparse_pages_[page][index % ENTITIES_PER_PAGE];
            }
        
            void assure_sparse_page(u32 index) {
                const usize page = index / ENTITIES_PER_PAGE;
        
                if (page >= page_count_) {
                    const usize new_page_count = page + 1;
                    auto** new_pages = static_cast<u32**>(
                        ::operator new(new_page_count * sizeof(u32*)));
                    
                    for (usize i = 0; i < page_count_; ++i) {
                        new_pages[i] = sparse_pages_[i];
                    }
                    for (usize i = page_count_; i < new_page_count; ++i) {
                        new_pages[i] = nullptr;
                    }
        
                    ::operator delete(sparse_pages_);
                    sparse_pages_ = new_pages;
                    page_count_ = new_page_count;
                }
        
                if (sparse_pages_[page] == nullptr) {
                    sparse_pages_[page] = static_cast<u32*>(
                        ::operator new(ENTITIES_PER_PAGE * sizeof(u32)));
                }
            }        
        public:
            static constexpr usize PAGE_SIZE = 4096;
            static constexpr usize ENTITIES_PER_PAGE = PAGE_SIZE / sizeof(u32);

            using value_type = T;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;

            // constructor
            explicit SparseSet(usize initial_capacity = 1024)
                : dense_entities_{nullptr},
                dense_components_{nullptr},
                sparse_pages_{nullptr},
                size_{0},
                capacity_{0},
                page_count_{0}
            {
                    reserve(initial_capacity);
            }

            ~SparseSet() {
                clear();
                ::operator delete(dense_entities_);
                ::operator delete(dense_components_);
                for (usize i = 0; i < page_count_; ++i) {
                    ::operator delete(sparse_pages_[i]);
                }
                ::operator delete(sparse_pages_);        
            }

            // move semantics (r-value)
            SparseSet(SparseSet&& other) noexcept
            : dense_entities_{other.dense_entities_}
            , dense_components_{other.dense_components_}
            , sparse_pages_{other.sparse_pages_}
            , size_{other.size_}
            , capacity_{other.capacity_}
            , page_count_{other.page_count_}
        {
            other.dense_entities_ = nullptr;
            other.dense_components_ = nullptr;
            other.sparse_pages_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            other.page_count_ = 0;    
        }

            SparseSet& operator=(SparseSet&& other) noexcept {
                if (this != &other) {
                    clear();
                    ::operator delete(dense_entities_);
                    ::operator delete(dense_components_);
                    for (usize i = 0; i < page_count_; ++i) {
                        ::operator delete(sparse_pages_[i]);
                    }
                    ::operator delete(sparse_pages_);    
                    
                    dense_entities_ = other.dense_entities_;
                    dense_components_ = other.dense_components_;
                    sparse_pages_ = other.sparse_pages_;
                    size_ = other.size_;
                    capacity_ = other.capacity_;
                    page_count_ = other.page_count_;
        
                    other.dense_entities_ = nullptr;
                    other.dense_components_ = nullptr;
                    other.sparse_pages_ = nullptr;
                    other.size_ = 0;
                    other.capacity_ = 0;
                    other.page_count_ = 0;        
                }
                return *this;
            }


            SparseSet(const SparseSet&) = delete;
            SparseSet& operator=(const SparseSet&) = delete;
            
            // capacity
            [[nodiscard]] auto size() const noexcept -> usize { return size_; }
            [[nodiscard]] auto capacity() const noexcept -> usize { return capacity_; }
            [[nodiscard]] auto empty() const noexcept -> bool { return size_ == 0; }

            void reserve(usize new_capacity) {
                if (new_capacity <= capacity_) {
                    return;
                }
        
                // Allocate new dense arrays
                auto* new_entities = static_cast<EntityID*>(
                    ::operator new(new_capacity * sizeof(EntityID), 
                                   std::align_val_t{TESSERA_CACHE_LINE_SIZE}));
                auto* new_components = static_cast<T*>(
                    ::operator new(new_capacity * sizeof(T), 
                                   std::align_val_t{alignof(T)}));
        
                // Move existing data
                for (usize i = 0; i < size_; ++i) {
                    new_entities[i] = dense_entities_[i];
                    std::construct_at(&new_components[i], std::move(dense_components_[i]));
                    std::destroy_at(&dense_components_[i]);
                }
        
                ::operator delete(dense_entities_);
                ::operator delete(dense_components_);
        
                dense_entities_ = new_entities;
                dense_components_ = new_components;
                capacity_ = new_capacity;
            }

            // element access
            [[nodiscard]] auto has(EntityID entity) const noexcept -> bool {
                const u32 index = entity_index(entity);
                const u32* sparse_idx = sparse_ptr(index);
                if (sparse_idx == nullptr) {
                    return false;
                }
                const u32 dense_idx = *sparse_idx;
                return dense_idx < size_ && dense_entities_[dense_idx] == entity;
            }
        
            [[nodiscard]] auto get(EntityID entity) -> reference {
                TESSERA_ASSERT(has(entity), "Entity does not exist in sparse set");
                const u32 dense_idx = *sparse_ptr(entity_index(entity));
                return dense_components_[dense_idx];
            }
        
            [[nodiscard]] auto get(EntityID entity) const -> const_reference {
                TESSERA_ASSERT(has(entity), "Entity does not exist in sparse set");
                const u32 dense_idx = *sparse_ptr(entity_index(entity));
                return dense_components_[dense_idx];
            }
        
            // modifiers
                /// Perfect-forwarding in-place construction of component for entity
            template <typename... Args>
                requires std::is_constructible_v<T, Args...>
            auto emplace(EntityID entity, Args&&... args) -> reference {
                TESSERA_ASSERT(!has(entity), "Entity already exists in sparse set");

                const u32 index = entity_index(entity);
                assure_sparse_page(index);

                if (size_ >= capacity_) {
                    reserve(capacity_ == 0 ? 64 : capacity_ * 2);
                }
    
                /*
                    Ensures that there are no holes within the arrays so that CPU doesn't 
                    waste more power for computing this
                */
                const u32 dense_idx = static_cast<u32>(size_); // figuring out where the end if the packed array is (size_)
                dense_entities_[dense_idx] = entity; // putting the entity ID at the end of the packed entity array
                std::construct_at(&dense_components_[dense_idx], std::forward<Args>(args)...); // putting the component data at the end of the packed data array

                *sparse_ptr(index) = dense_idx; // updating the directory to point to new packed index
                ++size_; // increase counter

                return dense_components_[dense_idx];
            }

            void remove(EntityID entity) {
                TESSERA_ASSERT(has(entity), "Entity does not exist in sparse set");

                const u32 index = entity_index(entity); 
                const u32 dense_idx = *sparse_ptr(index); // looks up where entity we want to delete is current at
                const u32 last_idx = static_cast<u32>(size_ - 1); // find the index of the very last item in our packed array

                if (dense_idx != last_idx) {
                    // grab  the entity ID of the very last item
                    EntityID last_entity = dense_entities_[last_idx];
                    
                    // overwrite the deleted entity's slow with the last entity's ID
                    dense_entities_[dense_idx] = last_entity;
                    // more overwriting entity's COMPONENT data with last entity's data
                    std::destroy_at(&dense_components_[dense_idx]);
                    std::construct_at(&dense_components_[dense_idx], 
                                    std::move(dense_components_[last_idx]));

                    // update directory so last entity knows new address
                    *sparse_ptr(entity_index(last_entity)) = dense_idx;
                }

                // destroy the data at the end of the array and shrink the size
                std::destroy_at(&dense_components_[last_idx]);
                --size_;
            }

            void clear() {
                for (usize i = 0; i < size_; ++i) {
                    std::destroy_at(&dense_components_[i]);
                }
                size_ = 0;
            }

            // dense-array views
            [[nodiscard]] auto data() noexcept -> std::span<T> {
                return {dense_components_, size_};
            }
        
            [[nodiscard]] auto data() const noexcept -> std::span<const T> {
                return {dense_components_, size_};
            }
        
            [[nodiscard]] auto entities() const noexcept -> std::span<const EntityID> {
                return {dense_entities_, size_};
            }
        
            // iterators
            [[nodiscard]] auto begin() noexcept -> pointer { return dense_components_; }
            [[nodiscard]] auto end() noexcept -> pointer { return dense_components_ + size_; }
            [[nodiscard]] auto begin() const noexcept -> const_pointer { return dense_components_; }
            [[nodiscard]] auto end() const noexcept -> const_pointer { return dense_components_ + size_; }
        
        };
}