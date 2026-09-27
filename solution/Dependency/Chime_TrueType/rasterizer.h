// Non-SDF rasterization is based on stb_truetype rasterizer by Sean Barrett: https://nothings.org/gamedev/rasterize/
// The key difference is that this library uses a simplified method for signed area calculation.

class rasterizer {
public:
	struct point {
		float x;
		float y;
	};
	struct edge {
		point points[2];
		bool isUpward;
		float InverseSlope() const {
			return (points[1].x - points[0].x) / (points[1].y - points[0].y);
		}
		float XCorrespondsToY(float y, float inverseSlope) const {
			return inverseSlope * (y - points[0].y) + points[0].x;
		}
	};
	struct activeEdge {
		const edge* pEdge;
		float inverseSlope;
		point points[2];
		activeEdge() = default;
		activeEdge(const edge& edge) :
			pEdge(&edge), inverseSlope(edge.InverseSlope()) {
			points[1] = pEdge->points[0];
		}
		const edge& operator*() const { return *pEdge; }
		const edge* operator->() const { return pEdge; }
		float HalfSlope() const {
			return inverseSlope == 0 ? 0 : 0.5f / inverseSlope;
		}
		float XCorrespondsToY(float y) const {
			return pEdge->XCorrespondsToY(y, inverseSlope);
		}
	};
	struct bounds {
		int16_t left;
		int16_t bottom;
		int16_t right;
		int16_t top;
	};
protected:
	std::vector<edge> edges;
	std::vector<activeEdge> activeEdges;
	std::vector<float> signedAreas;
	std::vector<float> rightwardFillings;
	/* Non-const Function */
	void PointsToEdges(const std::vector<point>& points) {
		edges.clear();
		const point* pPoint = points.data();
		const point* pBack = &points.back();
		for (; pPoint < pBack;)
			if (std::isnan(pPoint[1].x))
				pPoint += 2;
			else {
				bool isUpward = pPoint[1].y > pPoint[0].y;
				edges.push_back({ pPoint[isUpward], pPoint[!isUpward], isUpward });
				pPoint++;
			}
	}
	void SortEdges() {
		auto CountingSort = [](const uint8_t* pData, std::vector<uint32_t>& indices, uint32_t* indices_temp) {
			size_t count = indices.size();
			uint32_t sums[256]{};
			for (size_t i = 0; i < count; i++)
				sums[pData[sizeof(edge) * indices[i]]]++;
			for (size_t i = 1; i < 256; i++)
				sums[i] += sums[i - 1];
			std::memcpy(indices_temp, indices.data(), sizeof(uint32_t) * count);
			for (int32_t i = count - 1; i >= 0; i--)
				indices[--sums[pData[sizeof(edge) * indices_temp[i]]]] = indices_temp[i];
		};
		// Treat edge::p[0].y as uint32_t
		size_t count = edges.size();
		std::vector<uint32_t> indices(count);
		std::vector<uint32_t> temp(count);
		std::iota(indices.begin(), indices.end(), 0);
		if constexpr (std::endian::native == std::endian::little)
			for (size_t i = 0; i < sizeof(int32_t); i++)
				CountingSort(reinterpret_cast<const uint8_t*>(edges.data()) + offsetof(edge, points[0].y) + i, indices, temp.data());
		else
			for (int32_t i = sizeof(int32_t) - 1; i >= 0; i--)
				CountingSort(reinterpret_cast<const uint8_t*>(edges.data()) + offsetof(edge, points[0].y) + i, indices, temp.data());
		// Type of edge::p[0].y is float, reorder the indices
		size_t i;
		for (i = 0; i < count; i++)
			if (reinterpret_cast<const int32_t&>(edges[indices[i]].points[0].y) < 0)
				break;
		if (i != count) {
			std::memcpy(temp.data(), indices.data(), sizeof(uint32_t) * count);
			std::memcpy(indices.data() + count - i, temp.data(), sizeof(uint32_t) * i); // Move indices of positive values
			for (size_t j = 0, k = count - i; j < k; j++)                               // Reorder indices of negative values
				indices[j] = temp[count - 1 - j];
		}
		// Apply reverse indices
		edges.resize(count * 2); // Not reserve(...), Avoid UB
		edge* edges_copy = &edges[count];
		std::memcpy(edges_copy, edges.data(), sizeof(edge) * count);
		for (size_t i = 0; i < count; i++)
			edges[i] = edges_copy[indices[count - 1 - i]];
		edges.resize(count);
	}
	void UpdateActiveEdges(const edge*& pCurrentEdge, float rowTop) {
		// Check stored edges
		size_t count = activeEdges.size();
		activeEdge* pActiveEdge = activeEdges.data();
		size_t i = 0;
		for (; i < count; i++)
			if (activeEdges[i]->points[1].y < rowTop)
				*pActiveEdge++ = activeEdges[i];
		activeEdges.resize(pActiveEdge - activeEdges.data());
		// Add new edges
		const edge* pBack = &edges.back();
		float rowBottom = rowTop - 1;
		for (; pCurrentEdge <= pBack; pCurrentEdge++)
			if (pCurrentEdge->points[0].y <= rowBottom)
				break;
			else
				if (pCurrentEdge->points[0].y != pCurrentEdge->points[1].y)
					activeEdges.emplace_back(*pCurrentEdge);
	}
	void DrawGlyph(bounds glyphBoundingBox, uint8_t* pImageData, uint16_t imageLayerWidth, uint16_t imageLayerHeight, int16_t imageAscent) {
		uint32_t width = glyphBoundingBox.right - glyphBoundingBox.left;
		uint32_t widthPlusOne = width + 1;
		uint32_t height = glyphBoundingBox.top - glyphBoundingBox.bottom;
		signedAreas.clear();
		signedAreas.resize(width * imageLayerHeight);
		rightwardFillings.clear();
		rightwardFillings.resize(widthPlusOne * imageLayerHeight);
		for (auto& i : edges) {
			if (i.points[0].y == i.points[1].y)
				continue;
			int32_t edgeTop = int32_t(std::ceil(i.points[0].y));     // May be greater than glyphBoundingBox.top
			int32_t edgeBottom = int32_t(std::floor(i.points[1].y)); // May be less than glyphBoundingBox.bottom
			point points[2]; // topPoint, bottomPoint
			points[1] = i.points[0];
			float inverseSlope = i.InverseSlope();
			float absHalfSlope = std::abs(inverseSlope == 0 ? 0 : 0.5f / inverseSlope);
			bool leftPointIndex = inverseSlope >= 0;
			float* signedAreas = &this->signedAreas[(imageAscent - edgeTop) * width] - glyphBoundingBox.left;
			float* rightwardFillings = &this->rightwardFillings[(imageAscent - edgeTop) * widthPlusOne] - glyphBoundingBox.left;
			for (int32_t j = edgeTop; j > edgeBottom; signedAreas += width, rightwardFillings += widthPlusOne) {
				points[0] = points[1];
				points[1] = CalculateBottomPoint(i, inverseSlope, float(--j));
				int32_t x = int32_t(std::floor(points[leftPointIndex].x));
				int32_t x_right = int32_t(std::ceil(points[!leftPointIndex].x));
				float signedArea = 0;
				float signedArea_previous;
				if (i.isUpward) {
					for (; x < x_right; x++)
						signedArea_previous = signedArea,
						signedArea = CalculateArea(x + 1.f, points[leftPointIndex], points[!leftPointIndex], absHalfSlope),
						signedAreas[x] += signedArea - signedArea_previous;
					rightwardFillings[x_right] += points[0].y - points[1].y;
				}
				else {
					for (; x < x_right; x++)
						signedArea_previous = signedArea,
						signedArea = CalculateArea(x + 1.f, points[leftPointIndex], points[!leftPointIndex], absHalfSlope),
						signedAreas[x] -= signedArea - signedArea_previous;
					rightwardFillings[x_right] -= points[0].y - points[1].y;
				}
			}
		}
		float* signedAreas = &this->signedAreas[width * (imageAscent - glyphBoundingBox.top)];
		float* rightwardFillings = &this->rightwardFillings[widthPlusOne * (imageAscent - glyphBoundingBox.top)];
		for (size_t j = 0; j < height; j++, rightwardFillings++, pImageData += imageLayerWidth) {
			float filling = 0;
			for (size_t i = 0; i < width; i++, signedAreas++, rightwardFillings++)
				filling += *rightwardFillings,
				pImageData[i] = uint8_t(std::min(std::abs(filling + *signedAreas) * 255 + 0.5f, 255.f));
		}
	}
	void DrawGlyph_LineByLine(bounds glyphBoundingBox, uint8_t* pImageData, uint16_t imageLayerWidth) {
		SortEdges();
		const edge* pCurrentEdge = edges.data();
		activeEdges.clear();
		uint32_t width = glyphBoundingBox.right - glyphBoundingBox.left;
		signedAreas.resize(width * 2 + 1);
		float* signedAreas = this->signedAreas.data() - glyphBoundingBox.left;
		float* rightwardFillings = signedAreas + width;
		pImageData -= glyphBoundingBox.left;
		for (int32_t j = glyphBoundingBox.top; j > glyphBoundingBox.bottom; j--, pImageData += imageLayerWidth) {
			UpdateActiveEdges(pCurrentEdge, float(j));
			int32_t left = glyphBoundingBox.right;
			int32_t right = glyphBoundingBox.left;
			std::memset(this->signedAreas.data(), 0, sizeof(float) * width * 2);
			for (auto& i : activeEdges) {
				i.points[0] = i.points[1];
				i.points[1] = CalculateBottomPoint(*i, i.inverseSlope, j - 1.f);
				bool leftPointIndex = i.inverseSlope >= 0;
				float absHalfSlope = std::abs(i.HalfSlope());
				float signedArea = 0;
				float signedArea_previous;
				int32_t x = int32_t(std::floor(i.points[leftPointIndex].x));
				int32_t x_right = int32_t(std::ceil(i.points[!leftPointIndex].x));
				left = std::min(left, x);
				right = std::max(right, x_right);
				if (i->isUpward) {
					for (; x < x_right; x++)
						signedArea_previous = signedArea,
						signedArea = CalculateArea(x + 1.f, i.points[leftPointIndex], i.points[!leftPointIndex], absHalfSlope),
						signedAreas[x] += signedArea - signedArea_previous;
					rightwardFillings[x_right] += i.points[0].y - i.points[1].y;
				}
				else {
					for (; x < x_right; x++)
						signedArea_previous = signedArea,
						signedArea = CalculateArea(x + 1.f, i.points[leftPointIndex], i.points[!leftPointIndex], absHalfSlope),
						signedAreas[x] -= signedArea - signedArea_previous;
					rightwardFillings[x_right] -= i.points[0].y - i.points[1].y;
				}
			}
			float filling = 0;
			for (int32_t i = left; i < right; i++)
				filling += rightwardFillings[i],
				pImageData[i] = uint8_t(std::min(std::abs(filling + signedAreas[i]) * 255 + 0.5f, 255.f));
		}
	}
	void DrawGlyphSdf(bounds glyphBoundingBox, uint8_t* pImageData, uint16_t imageLayerWidth, uint16_t imageLayerHeight, int16_t imageAscent, uint8_t sdfPadding) {
		auto CalculateMagnitude = [](float x, float y) {
			return std::sqrt(x * x + y * y);
		};
		DrawGlyph(glyphBoundingBox, pImageData + sdfPadding * (1 + imageLayerWidth), imageLayerWidth, imageLayerHeight, imageAscent);
		float pixelDistanceScale = 128.f / sdfPadding;
		for (auto& i : edges) {
			auto& points = i.points;
			float vP0ToP1X = points[1].x - points[0].x;
			float vP0ToP1Y = points[1].y - points[0].y;
			float magnitude = CalculateMagnitude(vP0ToP1X, vP0ToP1Y);
			float tangentX = vP0ToP1X / magnitude;
			float tangentY = vP0ToP1Y / magnitude;
			int16_t top = int16_t(std::ceil(points[0].y + sdfPadding));
			int16_t bottom = int16_t(std::floor(points[1].y - sdfPadding));
			int16_t left = int16_t(std::floor(std::min(points[0].x, points[1].x) - sdfPadding));
			int16_t right = int16_t(std::ceil(std::max(points[0].x, points[1].x) + sdfPadding));
			auto DrawRange = [&](int32_t j, int32_t xBegin, int32_t xEnd) {
				for (int32_t i = xBegin; i < xEnd; i++) {
					float pixelCenterX = i + 0.5f;
					float pixelCenterY = j - 0.5f;
					float pixelToP0X = points[0].x - pixelCenterX;
					float pixelToP0Y = points[0].y - pixelCenterY;
					float distance = std::abs((tangentY * pixelToP0X - tangentX * pixelToP0Y) * pixelDistanceScale);
					float magnitudeOfOrthogonalProjectionToP0 = pixelToP0X * tangentX + pixelToP0Y * tangentY;
					uint8_t& pixelValue = pImageData[i - glyphBoundingBox.left + sdfPadding + (glyphBoundingBox.top - j + sdfPadding) * imageLayerWidth];
					if (pixelValue > 127) // Interior
						if (distance + 128 < 255)
							if (magnitudeOfOrthogonalProjectionToP0 > 0)
								pixelValue = std::min(int32_t(CalculateMagnitude(pixelToP0X, pixelToP0Y) * pixelDistanceScale + 128), int32_t(pixelValue));
							else if (magnitudeOfOrthogonalProjectionToP0 < -magnitude)
								pixelValue = std::min(int32_t(CalculateMagnitude(points[1].x - pixelCenterX, points[1].y - pixelCenterY) * pixelDistanceScale + 128), int32_t(pixelValue));
							else
								pixelValue = std::min(uint8_t(distance + 128), pixelValue);
						else;
					else                  // Exterior
						if (-distance + 128 > 0)
							if (magnitudeOfOrthogonalProjectionToP0 > 0)
								pixelValue = std::max(int32_t(-CalculateMagnitude(pixelToP0X, pixelToP0Y) * pixelDistanceScale + 128), int32_t(pixelValue));
							else if (magnitudeOfOrthogonalProjectionToP0 < -magnitude)
								pixelValue = std::max(int32_t(-CalculateMagnitude(points[1].x - pixelCenterX, points[1].y - pixelCenterY) * pixelDistanceScale + 128), int32_t(pixelValue));
							else
								pixelValue = std::max(uint8_t(-distance + 128), pixelValue);
				}
			};
			if (std::abs(vP0ToP1X) >= 1 &&
				vP0ToP1Y <= -1) {
				float inverseSlope = tangentX / tangentY;
				float sdfTopPointX = points[0].x + sdfPadding * inverseSlope;
				float horizontalSdfPadding = std::abs(sdfPadding / tangentY);
				float xBegin = sdfTopPointX - horizontalSdfPadding;
				float xEnd = sdfTopPointX + horizontalSdfPadding;
				for (int32_t j = top; j > bottom; j--) {
					DrawRange(j, std::max(int32_t(std::floor(xBegin)), int32_t(left)), std::min(int32_t(std::ceil(xEnd)), int32_t(right)));
					xBegin -= inverseSlope;
					xEnd -= inverseSlope;
				}
			}
			else
				for (int32_t j = top; j > bottom; j--)
					DrawRange(j, left, right);
		}
	}
	/* Static Function */
	static void QuadraticBezierToPolyline(point p0, point p1, point p2, std::vector<point>& points_out, float squarePrecision, uint32_t recursionLimit) {
		if (recursionLimit) {
			point mp = { // Midpoint
				(p0.x + p1.x * 2 + p2.x) / 4,
				(p0.y + p1.y * 2 + p2.y) / 4
			};
			float deltaX = (p0.x + p2.x) / 2 - mp.x;
			float deltaY = (p0.y + p2.y) / 2 - mp.y;
			if (squarePrecision < deltaX * deltaX + deltaY * deltaY) {
				QuadraticBezierToPolyline(p0, { (p0.x + p1.x) / 2, (p0.y + p1.y) / 2 }, mp, points_out, squarePrecision, recursionLimit - 1);
				QuadraticBezierToPolyline(mp, { (p1.x + p2.x) / 2, (p1.y + p2.y) / 2 }, p2, points_out, squarePrecision, recursionLimit - 1);
				return;
			}
		}
		points_out.push_back(p2);
	}
	static point CalculateBottomPoint(const edge& edge, float inverseSlope, float bottom) {
		if (edge.points[1].y >= bottom)
			return edge.points[1];
		return point{ edge.XCorrespondsToY(bottom, inverseSlope), bottom };
	}
	static float CalculateArea(float x, point leftmost, point rightmost, float absHalfSlope) {
		if (x > rightmost.x)
			return std::abs((x - (leftmost.x + rightmost.x) / 2) * (rightmost.y - leftmost.y));
		float deltaX = x - leftmost.x;
		return absHalfSlope * deltaX * deltaX;
	}
public:
	/* Non-const Function */
	void Rasterize(const std::vector<point>& points, bounds glyphBoundingBox, uint8_t* pImageData, uint16_t imageLayerWidth, uint16_t imageLayerHeight, int16_t imageAscent) {
		if (points.empty())
			return;
		PointsToEdges(points);
		if (!edges.empty())
			DrawGlyph(glyphBoundingBox, pImageData, imageLayerWidth, imageLayerHeight, imageAscent);
	}
	void Rasterize(const std::vector<point>& points, bounds glyphBoundingBox, uint8_t* pImageData, uint16_t imageLayerWidth) {
		if (points.empty())
			return;
		PointsToEdges(points);
		if (!edges.empty())
			DrawGlyph_LineByLine(glyphBoundingBox, pImageData, imageLayerWidth);
	}
	void RasterizeSdf(const std::vector<point>& points, bounds glyphBoundingBox, uint8_t* pImageData, uint16_t imageLayerWidth, uint16_t imageLayerHeight, int16_t imageAscent, uint8_t sdfPadding) {
		if (points.empty())
			return;
		PointsToEdges(points);
		if (!edges.empty())
			DrawGlyphSdf(glyphBoundingBox, pImageData, imageLayerWidth, imageLayerHeight, imageAscent, sdfPadding);
	}
	/* Static Function */
	static void TessellateContour(std::span<const glyfParser::pointF32> points_in, std::vector<rasterizer::point>& points_out, float squarePrecision) {
		auto GetOnCurvePoint = [](glyfParser::pointF32 p0 /* or p2 */, glyfParser::pointF32 p1) {
			return p0.isOnCurve ?
				rasterizer::point{ p0.x, p0.y } :
				rasterizer::point{ (p0.x + p1.x) / 2, (p0.y + p1.y) / 2 };
		};
		size_t firstPointIndex = points_out.size();
		for (size_t i = 0; i < points_in.size();)
			if (points_in[i].isOnCurve)
				points_out.emplace_back(points_in[i].x, points_in[i].y),
				i++;
			else {
				glyfParser::pointF32 nextPoint = points_in[(i + 1) % points_in.size()];
				rasterizer::QuadraticBezierToPolyline(
					GetOnCurvePoint(points_in[(i + points_in.size() - 1) % points_in.size()], points_in[i]),
					{ points_in[i].x, points_in[i].y },
					GetOnCurvePoint(nextPoint, points_in[i]),
					points_out, squarePrecision, 16);
				i += 1 + nextPoint.isOnCurve;
			}
		if (points_out[firstPointIndex].x != points_out.back().x ||
			points_out[firstPointIndex].y != points_out.back().y)
			points_out.push_back(points_out[firstPointIndex]);
		points_out.emplace_back(NAN, NAN);
	}
};