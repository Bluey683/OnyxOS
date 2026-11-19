#include "VhdxProvisioner.h"
namespace onyx::vm {
DiskProvisioningResult CreateVhdxForInstall(const DiskProvisioningRequest& r) {
if(r.profileId.empty()||r.destinationPath.empty())return {false,L"Profile and VHDX destination are required."};
if(r.sizeBytes<16ull*1024*1024*1024)return {false,L"Virtual disk must be at least 16 GiB."};
return {false,L"Automatic VHDX creation is not available yet: the referenced QEMU UWP runtime does not ship qemu-img or a disk-creation export. No empty file will be mislabeled as a valid VHDX."};
}
}