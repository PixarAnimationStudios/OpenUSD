class TextureRegistry {
  constructor(basename) {
    this.basename = basename;
    this.textures = [];
    this.loader = new THREE.TextureLoader();
  }
  getTexture(filename) {
    if (this.textures[filename]) {
      return this.textures[filename];
    }

    let textureResolve, textureReject;
    this.textures[filename] = new Promise((resolve, reject) => {
      textureResolve = resolve;
      textureReject = reject;
    });

    let resourcePath = filename;
    if (filename[0] !== '/') {
      resourcePath = this.basename + '[' + filename +']';
    }

    let filetype = undefined;
    if (filename.indexOf('.png') >= filename.length - 5) {
      filetype = 'image/png';
    } else if (filename.indexOf('.jpg') >= filename.length - 5) {
      filetype = 'image/jpeg';
    } else if (filename.indexOf('.jpeg') >= filename.length - 5) {
      filetype = 'image/jpeg';
    } else {
      throw new Error('Unknown filetype');
    }

    window.driver.getFile(resourcePath, (loadedFile) => {
      if (!loadedFile) {
        textureReject(new Error('Unknown file: ' + resourcePath));
        return;
      }

      let blob = new Blob([loadedFile.slice()], {type: filetype});
      let blobUrl = URL.createObjectURL(blob);

      // Load the texture
      this.loader.load(
        // resource URL
        blobUrl,

        // onLoad callback
        (texture) => {
          textureResolve(texture);
        },

        // onProgress callback currently not used
        undefined,

        // onError callback
        (err) => {
          textureReject(err);
        }
      );
    });

    return this.textures[filename];
  }
}

class HydraMesh {
  constructor(id, hydraInterface) {
    this._geometry = new THREE.BufferGeometry();
    this._id = id;
    this._interface = hydraInterface;
    this._points = undefined;
    this._normals = undefined;
    this._colors = undefined;
    this._uvs = undefined;
    this._indices = undefined;

    const material = new THREE.MeshPhysicalMaterial( {
      side: THREE.DoubleSide,
      color: new THREE.Color(0xa0a0a0) // a gray default color
    });

    this._mesh = new THREE.Mesh( this._geometry, material );
    this._mesh.castShadow = true;
    this._mesh.receiveShadow = true;

    window.scene.add(this._mesh);
  }

  updateOrder(attribute, attributeName, dimension = 3) {
    if (attribute && this._indices) {
      let values = [];
      for (let i = 0; i < this._indices.length; i++) {
        let index = this._indices[i]
        for (let j = 0; j < dimension; ++j) {
          values.push(attribute[dimension * index + j]);
        }
      }
      this._geometry.setAttribute( attributeName, new THREE.Float32BufferAttribute( values, dimension ) );
    }
  }

  updateIndices(indices) {
    this._indices = [];
    for (let i = 0; i< indices.length; i++) {
      this._indices.push(indices[i]);
    }
    this.updateOrder(this._points, 'position');
    this.updateOrder(this._normals, 'normal');
    if (this._colors) {
      this.updateOrder(this._colors, 'color');
    }
    if (this._uvs) {
      this.updateOrder(this._uvs, 'uv', 2);
      this._geometry.attributes.uv2 = this._geometry.attributes.uv;
    }
  }

  setTransform(matrix) {
    this._mesh.matrix.set(...matrix);
    this._mesh.matrix.transpose();
    this._mesh.matrixAutoUpdate = false;
  }

  updateNormals(normals) {
    this._normals = normals.slice();
    this.updateOrder(this._normals, 'normal');
  }

  // This is always called before prims are updated
  setMaterial(materialId) {
    console.log('Material: ' + materialId);
    if (this._interface.materials[materialId]) {
      this._mesh.material = this._interface.materials[materialId]._material;
    }
  }

  setDisplayColor(data, interpolation) {
    let wasDefaultMaterial = false;
    if (this._mesh.material === defaultMaterial) {
      this._mesh.material = this._mesh.material.clone();
      wasDefaultMaterial = true;
    }

    this._colors = null;

    if (interpolation === 'constant') {
      this._mesh.material.color = new THREE.Color().fromArray(data);
    } else if (interpolation === 'vertex') {
      // Per-vertex buffer attribute
      this._mesh.material.vertexColors = true;
      if (wasDefaultMaterial) {
        // Reset the debugging color
        this._mesh.material.color = new THREE.Color(0xffffff);
      }
      this._colors = data.slice();
      this.updateOrder(this._colors, 'color');
    } else {
      console.warn(`Unsupported displayColor interpolation type '${interpolation}'.`);
    }
  }

  setUV(data, dimension, interpolation) {
    // TODO: Support multiple UVs. For now, we simply set uv = uv2, which is required when a material has an aoMap.
    this._uvs = null;

    if (interpolation === 'facevarying') {
      // The UV buffer has already been prepared on the C++ side, so we just set it
      this._geometry.setAttribute('uv', new THREE.Float32BufferAttribute(data.slice(), dimension));
    } else if (interpolation === 'vertex') {
      // We have per-vertex UVs, so we need to sort them accordingly
      this._uvs = data.slice();
      this.updateOrder(this._uvs, 'uv', 2);
    }
    this._geometry.attributes.uv2 = this._geometry.attributes.uv;
  }

  updatePrimvar(name, data, dimension, interpolation) {
    if (name === 'points' || name === 'normals') {
      // Points and normals are set separately
      return;
    }

    console.log('Setting PrimVar: ' + name);

    // TODO: Support multiple UVs. For now, we simply set uv = uv2, which is required when a material has an aoMap.
    if (name.startsWith('st')) {
      name = 'uv';
    }

    switch(name) {
      case 'displayColor':
        this.setDisplayColor(data, interpolation);
        break;
      case 'uv':
        this.setUV(data, dimension, interpolation);
        break;
      default:
        console.warn('Unsupported primvar', name);
    }
  }

  updatePoints(points) {
    this._points = points.slice();
    this.updateOrder(this._points, 'position');
  }

  commit() {
    // Nothing to do here. All Three.js resources are already updated during the sync phase.
  }

}

let defaultMaterial;

class HydraMaterial {
  // Maps USD preview material texture names to Three.js MeshPhysicalMaterial names
  static usdPreviewToMeshPhysicalTextureMap = {
    'diffuseColor': 'map',
    'clearcoat': 'clearcoatMap',
    'clearcoatRoughness': 'clearcoatRoughnessMap',
    'emissiveColor': 'emissiveMap',
    'occlusion': 'aoMap',
    'roughness': 'roughnessMap',
    'metallic': 'metalnessMap',
    'normal': 'normalMap',
    'opacity': 'alphaMap'
  };

  static channelMap = {
    // Three.js expects many 8bit values such as roughness or metallness in a specific RGB texture channel.
    // We could write code to combine multiple 8bit texture files into different channels of one RGB texture where it
    // makes sense, but that would complicate this loader a lot. Most Three.js loaders don't seem to do it either.
    // Instead, we simply provide the 8bit image as an RGB texture, even though this might be less efficient.
    'r': THREE.RGBFormat,
    'rgb': THREE.RGBFormat,
    'rgba': THREE.RGBAFormat
  };

  // Maps USD preview material property names to Three.js MeshPhysicalMaterial names
  static usdPreviewToMeshPhysicalMap = {
    'clearcoat': 'clearcoat',
    'clearcoatRoughness': 'clearcoatRoughness',
    'diffuseColor': 'color',
    'emissiveColor': 'emissive',
    'ior': 'ior',
    'metallic': 'metalness',
    'opacity': 'opacity',
    'roughness': 'roughness',
  };

  constructor(id, hydraInterface) {
    this._id = id;
    this._nodes = {};
    this._interface = hydraInterface;
    if (!defaultMaterial) {
      defaultMaterial = new THREE.MeshPhysicalMaterial({
        side: THREE.DoubleSide,
        color: new THREE.Color(0xa0a0a0), // a gray default color
        envMap: window.envMap,
      });
    }
    this._material = defaultMaterial;
  }

  updateNode(networkId, path, parameters) {
    console.log('Updating Material Node: ' + networkId + ' ' + path);
    this._nodes[path] = parameters;
  }

  assignTexture(mainMaterial, parameterName) {
    const materialParameterMapName = HydraMaterial.usdPreviewToMeshPhysicalTextureMap[parameterName];
    if (materialParameterMapName === undefined) {
      console.warn(`Unsupported material texture parameter '${parameterName}'.`);
      return;
    }
    if (mainMaterial[parameterName] && mainMaterial[parameterName].nodeIn) {
      const textureFileName = mainMaterial[parameterName].nodeIn.file;
      const channel = mainMaterial[parameterName].inputName;

      // For debugging
      const matName = Object.keys(this._nodes).find(key => this._nodes[key] === mainMaterial);
      console.log(`Setting texture '${materialParameterMapName}' (${textureFileName}) of material '${matName}'...`);

      this._interface.registry.getTexture(textureFileName).then(texture => {
        if (materialParameterMapName === 'alphaMap') {
          // If this is an opacity map, check if it's using the alpha channel of the diffuse map.
          // If so, simply change the format of that diffuse map to RGBA and make the material transparent.
          // If not, we need to copy the alpha channel into a new texture's green channel, because that's what Three.js
          // expects for alpha maps (not supported at the moment).
          // NOTE that this only works if diffuse maps are always set before opacity maps, so the order of
          // 'assingTexture' calls for a material matters.
          if (textureFileName === mainMaterial.diffuseColor?.nodeIn?.file && channel === 'a') {
            this._material.map.format = THREE.RGBAFormat;
          } else {
            // TODO: Extract the alpha channel into a new RGB texture.
          }

          this._material.transparent = true;
          this._material.needsUpdate = true;
          return;
        } else if (materialParameterMapName === 'metalnessMap') {
          this._material.metalness = 1.0;
        } else if (materialParameterMapName === 'emissiveMap') {
          this._material.emissive = new THREE.Color(0xffffff);
        } else if (!HydraMaterial.channelMap[channel]) {
          console.warn(`Unsupported texture channel '${channel}'!`);
          return;
        }

        // Clone texture and set the correct format.
        const clonedTexture = texture.clone();
        clonedTexture.format = HydraMaterial.channelMap[channel];
        clonedTexture.needsUpdate = true;
        clonedTexture.wrapS = THREE.RepeatWrapping;
        clonedTexture.wrapT = THREE.RepeatWrapping;

        this._material[materialParameterMapName] = clonedTexture;

        this._material.needsUpdate = true;
      });
    } else {
      this._material[materialParameterMapName] = undefined;
    }
  }

  assignProperty(mainMaterial, parameterName) {
    const materialParameterName = HydraMaterial.usdPreviewToMeshPhysicalMap[parameterName];
    if (materialParameterName === undefined) {
      console.warn(`Unsupported material parameter '${parameterName}'.`);
      return;
    }
    if (mainMaterial[parameterName] !== undefined && !mainMaterial[parameterName].nodeIn) {
      console.log(`Assigning property ${parameterName}: ${mainMaterial[parameterName]}`);
      if (Array.isArray(mainMaterial[parameterName])) {
        this._material[materialParameterName] = new THREE.Color().fromArray(mainMaterial[parameterName]);
      } else {
        this._material[materialParameterName] = mainMaterial[parameterName];
        if (materialParameterName === 'opacity' && mainMaterial[parameterName] < 1.0) {
          this._material.transparent = true;
        }
      }
    }
  }

  updateFinished(type, relationships) {
    for (let relationship of relationships) {
      relationship.nodeIn = this._nodes[relationship.inputId];
      relationship.nodeOut = this._nodes[relationship.outputId];
      relationship.nodeIn[relationship.inputName] = relationship;
      relationship.nodeOut[relationship.outputName] = relationship;
    }
    console.log('Finalizing Material: ' + this._id);

    // find the main material node
    let mainMaterialNode = undefined;
    for (let node of Object.values(this._nodes)) {
      if (node.diffuseColor) {
        mainMaterialNode = node;
        break;
      }
    }

    if (!mainMaterialNode) {
      this._material = defaultMaterial;
      return;
    }

    // TODO: Ideally, we don't recreate the material on every update.
    // Creating a new one requires to also update any meshes that reference it. So we're relying on the C++ side to
    // call this before also calling `setMaterial` on the affected meshes.
    console.log('Creating Material: ' + this._id);
    this._material = new THREE.MeshPhysicalMaterial({});

    // Assign textures
    for (let key in HydraMaterial.usdPreviewToMeshPhysicalTextureMap) {
      this.assignTexture(mainMaterialNode, key);
    }

    // Assign material properties
    for (let key in HydraMaterial.usdPreviewToMeshPhysicalMap) {
      this.assignProperty(mainMaterialNode, key);
    }

    if (window.envMap) {
      this._material.envMap = window.envMap;
    }

    console.log(this._material);
  }
}

export class RenderDelegateInterface {

  constructor(filename) {
    this.registry = new TextureRegistry(filename);
    this.materials = {};
    this.meshes = {};
  }

  createRPrim(typeId, id, instancerId) {
    console.log('Creating RPrim: ' + typeId + ' ' + id);
    let mesh = new HydraMesh(id, this);
    this.meshes[id] = mesh;
    return mesh;
  }

  createBPrim(typeId, id) {
    console.warn('BPrims are not supported!');
  }

  createSPrim(typeId, id) {
    console.log('Creating SPrim: ' + typeId + ' ' + id);

    if (typeId === 'material') {
      let material = new HydraMaterial(id, this);
      this.materials[id] = material;
      return material;
    } else {
      return undefined;
    }
  }

  CommitResources() {
    for (const id in this.meshes) {
        const hydraMesh = this.meshes[id]
        hydraMesh.commit();
    }
  }
}
