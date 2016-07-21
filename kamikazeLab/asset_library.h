#ifndef _ASSET_LIBRARY_H_
#define _ASSET_LIBRARY_H_

#include <unordered_map>
#include <string>
#include <cassert>

template<typename CpuAssetType, typename GpuAssetType, typename IDType = std::string>
struct AssetLibrary
{
	void add(const IDType& id, const CpuAssetType& cpuAsset);
	GpuAssetType get(const IDType& id)const;
	GpuAssetType remove(const IDType& id);
	void removeAndRelease(const IDType& id);
	bool exists(const IDType& id)const;

private:
	std::unordered_map<IDType, GpuAssetType> assets;
};

template<typename CpuAssetType, typename GpuAssetType, typename IDType>
inline void AssetLibrary<CpuAssetType, GpuAssetType, IDType>::add(const IDType& id, const CpuAssetType& cpuAsset)
{
	auto it = assets.find(id);
	assert(it == assets.end());
	assets.insert(std::pair<IDType, GpuAssetType>(id, cpuAsset.uploadToGPU()));
}

template<typename CpuAssetType, typename GpuAssetType, typename IDType>
inline GpuAssetType AssetLibrary<CpuAssetType, GpuAssetType, IDType>::get(const IDType& id)const
{
	auto it = assets.find(id);
	assert(it != assets.end());
	return it->second;
}

template<typename CpuAssetType, typename GpuAssetType, typename IDType>
inline GpuAssetType AssetLibrary<CpuAssetType, GpuAssetType, IDType>::remove(const IDType& id)
{
	auto it = assets.find(id);
	assert(it != assets.end());
	GpuAssetType asset = std::move(it->second);
	assets.erase(it);
	return asset;
}

template<typename CpuAssetType, typename GpuAssetType, typename IDType>
inline void AssetLibrary<CpuAssetType, GpuAssetType, IDType>::removeAndRelease(const IDType& id)
{
	GpuAssetType asset = remove(id);
	asset.release();
}

template<typename CpuAssetType, typename GpuAssetType, typename IDType>
inline bool AssetLibrary<CpuAssetType, GpuAssetType, IDType>::exists(const IDType& id)const
{
	return assets.find(id) != assets.end();
}

#endif