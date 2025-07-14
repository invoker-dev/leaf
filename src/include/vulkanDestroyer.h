#include <VkBootstrap.h>
#include <leafStructs.h>
class VulkanDestroyer { // DOOM!
public:
  void addImage(AllocatedImage image);
  void addBuffer(VkBuffer buffer);
  void addCommandPool(VkCommandPool pool);
  void addSemaphore(VkSemaphore semaphore);
  void addFence(VkFence fence);
  void addDescriptorPool(VkDescriptorPool pool);
  void addDescriptorSetLayout(VkDescriptorSetLayout layout);

  void flush(VkDevice device, VmaAllocator allocator);

private:
  std::vector<AllocatedImage>        images;
  std::vector<VkBuffer>              buffers;
  std::vector<VkCommandPool>         commandPools;
  std::vector<VkSemaphore>           semaphores;
  std::vector<VkFence>               fences;
  std::vector<VkDescriptorPool>      descriptorPools;
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};
