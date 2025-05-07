#pragma once

#include <vector>

#include "camera.hpp"
#include "spotlight.hpp"
#include "framebuffer.hpp"
#include "shadowmap_framebuffer.hpp"
#include "ogl_material_factory.hpp"
#include "ogl_geometry_factory.hpp"

class QuadRenderer {
public:
	QuadRenderer()
		: mQuad(generateQuadTex())
	{}

	void render(const OGLShaderProgram &aShaderProgram, MaterialParameterValues &aParameters) const {
		aShaderProgram.use();
		aShaderProgram.setMaterialParameters(aParameters, MaterialParameterValues());
		GL_CHECK(glBindVertexArray(mQuad.vao.get()));
  		GL_CHECK(glDrawElements(mQuad.mode, mQuad.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(0)));
	}
protected:

	IndexedBuffer mQuad;
};

inline std::vector<CADescription> getColorNormalPositionAttachments() {
	return {
		{ GL_RGBA, GL_FLOAT, GL_RGBA },
		// To store values outside the range [0,1] we need different internal format then normal GL_RGBA
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
		{ GL_RED, GL_FLOAT, GL_RED }
	};
}

inline std::vector<CADescription> getSingleColorAttachment() {
	return {
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
	};
}

inline std::vector<CADescription> getDoubleColorAttachment() {
	return {
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
		{ GL_RED, GL_FLOAT, GL_RED },
	};
}

class Renderer {
public:
	Renderer(OGLMaterialFactory &aMaterialFactory)
		: mMaterialFactory(aMaterialFactory)
	{
		mCompositingShader = std::static_pointer_cast<OGLShaderProgram>(
				mMaterialFactory.getShaderProgram("compositing"));
		mShadowMapShader = std::static_pointer_cast<OGLShaderProgram>(
			mMaterialFactory.getShaderProgram("shadowmap"));
		mDoFShader = std::static_pointer_cast<OGLShaderProgram>(
			mMaterialFactory.getShaderProgram("depthOfField"));
	}

	void initialize(int aWidth, int aHeight) {
		mWidth = aWidth;
		mHeight = aHeight;
		GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));

		mFramebuffer = std::make_unique<Framebuffer>(aWidth, aHeight, getColorNormalPositionAttachments());
		mDoFFramebuffer = std::make_unique<Framebuffer>(aWidth, aHeight, getDoubleColorAttachment());
		mShadowmapFramebuffer = std::make_unique<Framebuffer>(600, 600, getSingleColorAttachment());
	}

	void clear() {
		mFramebuffer->bind();
		GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	}

	template<typename TScene, typename TCamera>
	void geometryPass(const TScene &aScene, const TCamera &aCamera, RenderOptions aRenderOptions) {
		GL_CHECK(glEnable(GL_DEPTH_TEST));
		GL_CHECK(glViewport(0, 0, mWidth, mHeight));
		mFramebuffer->bind();
		mFramebuffer->setDrawBuffers();
		auto projection = aCamera.getProjectionMatrix();
		auto view = aCamera.getViewMatrix();

		std::vector<RenderData> renderData;
		for (const auto &object : aScene.getObjects()) {
			auto data = object.getRenderData(aRenderOptions);
			if (data) {
				renderData.push_back(data.value());
			}
		}

		MaterialParameterValues fallbackParameters;
		fallbackParameters["u_projMat"] = projection;
		fallbackParameters["u_viewMat"] = view;
		fallbackParameters["u_solidColor"] = glm::vec4(0,0,0,1);
		fallbackParameters["u_viewPos"] = aCamera.getPosition();
		for (const auto &data: renderData) {
			const glm::mat4 modelMat = data.modelMat;
			const MaterialParameters &params = data.mMaterialParams;
			const OGLShaderProgram &shaderProgram = static_cast<const OGLShaderProgram &>(data.mShaderProgram);
			const OGLGeometry &geometry = static_cast<const OGLGeometry&>(data.mGeometry);

			fallbackParameters["u_modelMat"] = modelMat;
			fallbackParameters["u_normalMat"] = glm::mat3(modelMat);

			shaderProgram.use();
			shaderProgram.setMaterialParameters(params.mParameterValues, fallbackParameters);

			geometry.bind();
			geometry.draw();
		}
		mFramebuffer->unbind();
	}

	template<typename TLight>
	void compositingPass(const TLight &aLight) {
		mDoFFramebuffer->bind();
		mDoFFramebuffer->setDrawBuffers();
		GL_CHECK(glDisable(GL_DEPTH_TEST));
		GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));

		MaterialParameterValues compParams{
			{ "u_diffuse", TextureInfo("diffuse", mFramebuffer->getColorAttachment(0)) },
			{ "u_normal", TextureInfo("diffuse", mFramebuffer->getColorAttachment(1)) },
			{ "u_position", TextureInfo("diffuse", mFramebuffer->getColorAttachment(2)) },
			{ "u_depth", TextureInfo("diffuse", mFramebuffer->getColorAttachment(3)) },
			{ "u_shadowMap", TextureInfo("shadowMap", mShadowmapFramebuffer->getColorAttachment(0)) },
			{ "u_lightPos", aLight.getPosition() },
			{ "u_lightMat", aLight.getViewMatrix() },
			{ "u_lightProjMat", aLight.getProjectionMatrix() }
		};

		mQuadRenderer.render(*mCompositingShader, compParams);

		mDoFFramebuffer->unbind();
	}

	template<typename TScene, typename TLight>
	void shadowMapPass(const TScene &aScene, const TLight &aLight) {
		GL_CHECK(glEnable(GL_DEPTH_TEST));
		mShadowmapFramebuffer->bind();
		GL_CHECK(glViewport(0, 0, 600, 600));
		GL_CHECK(glClearColor(1.0f, 1.0f, 1.0f, 1.0f));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		mShadowmapFramebuffer->setDrawBuffers();
		auto projection = aLight.getProjectionMatrix();
		auto view = aLight.getViewMatrix();

		MaterialParameterValues fallbackParameters;
		fallbackParameters["u_projMat"] = projection;
		fallbackParameters["u_viewMat"] = view;
		fallbackParameters["u_viewPos"] = aLight.getPosition();

		std::vector<RenderData> renderData;
		RenderOptions renderOptions = {"solid"};
		for (const auto &object : aScene.getObjects()) {
			auto data = object.getRenderData(renderOptions);
			if (data) {
				renderData.push_back(data.value());
			}
		}
		mShadowMapShader->use();
		for (const auto &data: renderData) {
			const glm::mat4 modelMat = data.modelMat;
			const MaterialParameters &params = data.mMaterialParams;
			const OGLShaderProgram &shaderProgram = static_cast<const OGLShaderProgram &>(data.mShaderProgram);
			const OGLGeometry &geometry = static_cast<const OGLGeometry&>(data.mGeometry);

			fallbackParameters["u_modelMat"] = modelMat;
			fallbackParameters["u_normalMat"] = glm::mat3(modelMat);

			mShadowMapShader->setMaterialParameters(fallbackParameters, {});

			geometry.bind();
			geometry.draw();
		}



		mShadowmapFramebuffer->unbind();
	}

	void depthOfFieldPass(float focusDistance, float focusRange) {
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

    MaterialParameterValues dofParameters = {
        { "u_sceneTexture", TextureInfo("sceneTexture", mDoFFramebuffer->getColorAttachment(0)) },
        { "u_depthTexture", TextureInfo("depthTexture", mDoFFramebuffer->getColorAttachment(1)) },
        { "u_focusDistance", focusDistance },
        { "u_focusRange", focusRange },
    };

    mQuadRenderer.render(*mDoFShader, dofParameters);
	}

protected:
	int mWidth = 100;
	int mHeight = 100;

	std::unique_ptr<Framebuffer> mFramebuffer;
	std::unique_ptr<Framebuffer> mDoFFramebuffer;
	std::unique_ptr<Framebuffer> mShadowmapFramebuffer;

	QuadRenderer mQuadRenderer;

	std::shared_ptr<OGLShaderProgram> mCompositingShader;
	std::shared_ptr<OGLShaderProgram> mShadowMapShader;
	std::shared_ptr<OGLShaderProgram> mDoFShader;

	OGLMaterialFactory &mMaterialFactory;
};
