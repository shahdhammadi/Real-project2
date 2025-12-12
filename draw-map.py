import sys
import os
import math

try:
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    import numpy as np

    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("⚠️ matplotlib not installed. 3D visualization will be disabled.")
    print("💡 Install with: pip install matplotlib")


class MapVisualizer:
    def __init__(self, map_file_path):
        self.map_file_path = map_file_path
        self.survivors = []
        self.obstacles = []
        self.width = 0
        self.height = 0
        self.depth = 0

    def parse_map_file(self):
        """x,y,z,type,priority,heat,co2,confidence,location"""
        print(f"📂 Loading map from: {self.map_file_path}")

        if not os.path.exists(self.map_file_path):
            print(f"❌ Error: File '{self.map_file_path}' not found!")
            return False

        try:
            with open(self.map_file_path, "r", encoding="utf-8") as file:
                lines = file.readlines()

            # Read Dimensions...
            expected_survivors = 0
            for line in lines:
                line = line.strip()
                if line.startswith("WIDTH="):
                    self.width = int(line.split("=")[1].strip())
                elif line.startswith("HEIGHT="):
                    self.height = int(line.split("=")[1].strip())
                elif line.startswith("DEPTH="):
                    self.depth = int(line.split("=")[1].strip())
                elif line.startswith("SURVIVORS="):
                    expected_survivors = int(line.split("=")[1].strip())

            # Read fata of Cell
            for line in lines:
                line = line.strip()

                if (
                    line.startswith("WIDTH=")
                    or line.startswith("HEIGHT=")
                    or line.startswith("DEPTH=")
                    or line.startswith("SURVIVORS=")
                    or line.startswith("#")
                    or line == ""
                ):
                    continue

                if "," in line:
                    parts = line.strip().split(",")

                    if len(parts) >= 4:
                        try:
                            x, y, z, cell_type = map(int, parts[:4])

                            if cell_type == 1:  # Obstacel
                                self.obstacles.append((x, y, z))

                            elif cell_type == 2:  # survivor
                                #  x,y,z,2,priority,heat,co2,confidence,location
                                if len(parts) >= 8:
                                    survivor_data = {
                                        "position": (x, y, z),
                                        "priority": int(parts[4]),  # constant 5
                                        "heat": float(parts[5]),  #  heat
                                        "co2": float(parts[6]),  #  CO2
                                        "confidence": int(parts[7]),  # confidence
                                        "location_type": (
                                            parts[8] if len(parts) > 8 else "unknown"
                                        ),
                                    }
                                    self.survivors.append(survivor_data)
                        except (ValueError, IndexError) as e:
                            continue

            print(f"✅ Map loaded successfully:")
            print(f"   Dimensions: {self.width} × {self.height} × {self.depth}")
            print(f"   Obstacles: {len(self.obstacles)}")
            print(f"   Survivors: {len(self.survivors)}")
            print(f"   Expected survivors: {expected_survivors}")
            return True

        except Exception as e:
            print(f"❌ Error loading map file: {str(e)}")
            import traceback

            traceback.print_exc()
            return False

    def create_3d_collapsed_building(self):
        """رسم مبنى ثلاثي الأبعاد مع عرض الناجين بناءً على الحرارة وثاني أكسيد الكربون"""
        if not HAS_MATPLOTLIB:
            print("❌ matplotlib is required for 3D visualization!")
            return

        print("\n🏢 Creating 3D Collapsed Building Visualization...")

        fig = plt.figure(figsize=(16, 12))
        ax = fig.add_subplot(111, projection="3d")

        # رسم هيكل المبنى
        if self.width > 0 and self.height > 0 and self.depth > 0:
            # إنشاء إطار المبنى
            building_color = "#8B7355"  # لون بني للمبنى
            building_alpha = 0.1  # شفافية منخفضة

            # رسم الجدران الخارجية
            for z in range(self.depth + 1):
                # الأرضية والسقف
                x = [0, self.width, self.width, 0, 0]
                y = [0, 0, self.height, self.height, 0]
                ax.plot(
                    x,
                    y,
                    [z, z, z, z, z],
                    color=building_color,
                    linewidth=1,
                    alpha=building_alpha * 2,
                )

            # رسم الأعمدة
            for x in [0, self.width]:
                for y in [0, self.height]:
                    ax.plot(
                        [x, x],
                        [y, y],
                        [0, self.depth],
                        color=building_color,
                        linewidth=1.5,
                        alpha=building_alpha * 2,
                    )

        # رسم الأنقاض (العوائق)
        if self.obstacles:
            obs_x, obs_y, obs_z = [], [], []
            obs_sizes = []

            for x, y, z in self.obstacles:
                obs_x.append(x)
                obs_y.append(y)
                obs_z.append(z)
                # حجم الأنقاض يختلف حسب الكثافة
                obs_sizes.append(20 + (z * 5))  # الأنقاض في الطوابق السفلية أكبر

            ax.scatter(
                obs_x,
                obs_y,
                obs_z,
                c="#654321",  # لون بني للأنقاض
                s=obs_sizes,
                alpha=0.4,
                marker="h",  # شكل سداسي للأنقاض
                edgecolors="#3D2B1F",
                linewidth=0.5,
                label=f"Debris ({len(self.obstacles)})",
                depthshade=True,
            )

        # رسم الناجين مع تمييز حسب الحرارة وثاني أكسيد الكربون
        if self.survivors:
            survivors_data = {
                "position": [],
                "heat": [],
                "co2": [],
                "colors": [],
                "sizes": [],
            }

            for survivor in self.survivors:
                x, y, z = survivor["position"]
                survivors_data["position"].append((x, y, z))
                survivors_data["heat"].append(survivor["heat"])
                survivors_data["co2"].append(survivor["co2"])

                # تحديد اللون بناءً على الحرارة
                heat = survivor["heat"]
                if heat < 36.0:
                    color = "#1E90FF"  # أزرق فاتح - درجة حرارة منخفضة
                elif heat < 37.5:
                    color = "#32CD32"  # أخضر - درجة حرارة طبيعية
                else:
                    color = "#FF4500"  # أحمر برتقالي - درجة حرارة مرتفعة

                # تحديد الحجم بناءً على ثاني أكسيد الكربون
                co2 = survivor["co2"]
                if co2 < 500:
                    size = 120  # حجم صغير لـ CO2 منخفض
                elif co2 < 1000:
                    size = 180  # حجم متوسط
                else:
                    size = 250  # حجم كبير لـ CO2 مرتفع

                survivors_data["colors"].append(color)
                survivors_data["sizes"].append(size)

            # فصل النقاط لإنشاء وسيلة إيضاح منفصلة
            low_temp = [
                (pos, color, size)
                for pos, color, size, heat in zip(
                    survivors_data["position"],
                    survivors_data["colors"],
                    survivors_data["sizes"],
                    survivors_data["heat"],
                )
                if heat < 36.0
            ]
            normal_temp = [
                (pos, color, size)
                for pos, color, size, heat in zip(
                    survivors_data["position"],
                    survivors_data["colors"],
                    survivors_data["sizes"],
                    survivors_data["heat"],
                )
                if 36.0 <= heat < 37.5
            ]
            high_temp = [
                (pos, color, size)
                for pos, color, size, heat in zip(
                    survivors_data["position"],
                    survivors_data["colors"],
                    survivors_data["sizes"],
                    survivors_data["heat"],
                )
                if heat >= 37.5
            ]

            # رسم كل مجموعة على حدة مع تسمية مختلفة
            if low_temp:
                x_vals, y_vals, z_vals = zip(*[pos for pos, _, _ in low_temp])
                colors = [color for _, color, _ in low_temp]
                sizes = [size for _, _, size in low_temp]
                ax.scatter(
                    x_vals,
                    y_vals,
                    z_vals,
                    c=colors,
                    s=sizes,
                    alpha=0.9,
                    marker="o",
                    edgecolors="black",
                    linewidth=1.5,
                    label="Low Temp (<36°C)",
                    depthshade=True,
                )

            if normal_temp:
                x_vals, y_vals, z_vals = zip(*[pos for pos, _, _ in normal_temp])
                colors = [color for _, color, _ in normal_temp]
                sizes = [size for _, _, size in normal_temp]
                ax.scatter(
                    x_vals,
                    y_vals,
                    z_vals,
                    c=colors,
                    s=sizes,
                    alpha=0.9,
                    marker="o",
                    edgecolors="black",
                    linewidth=1.5,
                    label="Normal Temp (36-37.5°C)",
                    depthshade=True,
                )

            if high_temp:
                x_vals, y_vals, z_vals = zip(*[pos for pos, _, _ in high_temp])
                colors = [color for _, color, _ in high_temp]
                sizes = [size for _, _, size in high_temp]
                ax.scatter(
                    x_vals,
                    y_vals,
                    z_vals,
                    c=colors,
                    s=sizes,
                    alpha=0.9,
                    marker="o",
                    edgecolors="black",
                    linewidth=1.5,
                    label="High Temp (>37.5°C)",
                    depthshade=True,
                )

        # إعداد المحاور والتسميات
        ax.set_xlabel("X Position (m)", fontsize=12, fontweight="bold")
        ax.set_ylabel("Y Position (m)", fontsize=12, fontweight="bold")
        ax.set_zlabel("Floor Level", fontsize=12, fontweight="bold")

        # إضافة شبكة
        ax.grid(True, alpha=0.2, linestyle="--")

        # ضبط حدود المحاور
        ax.set_xlim([0, max(self.width, 1)])
        ax.set_ylim([0, max(self.height, 1)])
        ax.set_zlim([0, max(self.depth, 1)])

        # ضبط زاوية الرؤية
        ax.view_init(elev=30, azim=45)

        # إضافة العنوان الرئيسي
        plt.suptitle(
            "3D Visualization of Collapsed Building with Survivors",
            fontsize=18,
            fontweight="bold",
            y=0.96,
        )

        # إضافة عنوان فرعي
        ax.set_title(
            f"Color indicates body temperature | Size indicates CO₂ level\n"
            f"Building Dimensions: {self.width}×{self.height}×{self.depth}m | "
            f"Survivors: {len(self.survivors)} | Debris: {len(self.obstacles)}",
            fontsize=12,
            pad=20,
        )

        # إضافة وسيلة الإيضاح
        ax.legend(loc="upper right", fontsize=10, framealpha=0.9, markerscale=0.8)

        # إضافة مربع معلومات
        if self.survivors:
            # حساب الإحصائيات
            avg_heat = np.mean([s["heat"] for s in self.survivors])
            avg_co2 = np.mean([s["co2"] for s in self.survivors])
            max_heat = np.max([s["heat"] for s in self.survivors])
            min_heat = np.min([s["heat"] for s in self.survivors])

            info_lines = [
                "╔══════════════════════════════════╗",
                "║      SURVIVOR STATISTICS         ║",
                "╠══════════════════════════════════╣",
                f"║  Avg Temp: {avg_heat:.1f}°C      ║",
                f"║  Temp Range:                     ║ ",
                f"║ {min_heat:.1f}-{max_heat:.1f}°C  ║",
                "║  Avg CO₂: {avg_co2:.0f} ppm      ║",
                "╠══════════════════════════════════╣",
                "║        COLOR LEGEND:             ║",
                "║   Blue: Low Temp (<36°C)         ║",
                "║   Green: Normal Temp             ║",
                "║   Red: High Temp (>37.5°C)       ║",
                "╠══════════════════════════════════╣",
                "║        SIZE INDICATES:           ║",
                "║  • Small: CO₂ < 500 ppm          ║",
                "║  • Medium: 500-1000 ppm          ║",
                "║  • Large: CO₂ > 1000 ppm         ║",
                "╚══════════════════════════════════╝",
            ]

            info_text = "\n".join(info_lines)

            ax.text2D(
                0.02,
                0.98,
                info_text,
                transform=ax.transAxes,
                fontsize=8,
                fontfamily="monospace",
                verticalalignment="top",
                bbox=dict(
                    boxstyle="round",
                    facecolor="lightyellow",
                    alpha=0.9,
                    edgecolor="goldenrod",
                    linewidth=1.5,
                ),
            )

        plt.tight_layout()

        output_file = "3D_CollapsedBuilding.png"
        plt.savefig(output_file, dpi=300, bbox_inches="tight")
        plt.close(fig)

        print(f"📸 3D collapsed building visualization saved to: {output_file}")
        print(f"   • Survivors colored by temperature")
        print(f"   • Size indicates CO₂ level")
        print(f"   • Building dimensions: {self.width}x{self.height}x{self.depth}")

    def create_floor_distribution_chart(self):
        """رسم توزيع الناجين والأنقاض على كل طابق"""
        if not HAS_MATPLOTLIB or not self.survivors:
            print("⚠️ matplotlib or survivors data required!")
            return

        print("\n📊 Creating Floor Distribution Chart...")

        # حساب التوزيع الرأسي
        floor_counts = {}
        for z in range(self.depth):
            survivors_on_floor = len(
                [s for s in self.survivors if s["position"][2] == z]
            )
            obstacles_on_floor = len([obs for obs in self.obstacles if obs[2] == z])
            floor_area = (
                self.width * self.height if self.width > 0 and self.height > 0 else 1
            )
            floor_counts[z] = {
                "survivors": survivors_on_floor,
                "obstacles": obstacles_on_floor,
                "density": obstacles_on_floor / floor_area,
            }

        # رسم المخطط
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

        floors = list(floor_counts.keys())
        survivors_count = [floor_counts[f]["survivors"] for f in floors]
        obstacles_count = [floor_counts[f]["obstacles"] for f in floors]

        # المخطط الأول: توزيع الناجين والأنقاض
        width = 0.35
        x = np.arange(len(floors))

        ax1.bar(
            x - width / 2,
            survivors_count,
            width,
            label="Survivors",
            color="red",
            alpha=0.7,
        )
        ax1.bar(
            x + width / 2,
            obstacles_count,
            width,
            label="Debris",
            color="brown",
            alpha=0.7,
        )
        ax1.set_xlabel("Floor Level")
        ax1.set_ylabel("Count")
        ax1.set_title("Distribution of Survivors and Debris per Floor")
        ax1.set_xticks(x)
        ax1.set_xticklabels([f"Floor {f}" for f in floors])
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # إضافة الأرقام على الأعمدة
        for i, v in enumerate(survivors_count):
            ax1.text(i - width / 2, v + 0.5, str(v), ha="center")
        for i, v in enumerate(obstacles_count):
            ax1.text(i + width / 2, v + 0.5, str(v), ha="center")

        # المخطط الثاني: كثافة الأنقاض
        densities = [floor_counts[f]["density"] for f in floors]
        colors = [
            "green" if d < 0.3 else "orange" if d < 0.6 else "red" for d in densities
        ]

        ax2.bar(floors, densities, color=colors, alpha=0.7)
        ax2.set_xlabel("Floor Level")
        ax2.set_ylabel("Debris Density (obstacles/m²)")
        ax2.set_title("Debris Density per Floor")
        ax2.set_xticks(floors)
        ax2.set_xticklabels([f"Floor {f}" for f in floors])
        ax2.grid(True, alpha=0.3)

        # خطوط التوجيه
        ax2.axhline(
            y=0.3, color="green", linestyle="--", alpha=0.5, label="Low Density"
        )
        ax2.axhline(
            y=0.6, color="orange", linestyle="--", alpha=0.5, label="Medium Density"
        )
        ax2.axhline(y=0.9, color="red", linestyle="--", alpha=0.5, label="High Density")
        ax2.legend()

        plt.suptitle(
            f"Vertical Distribution Analysis - Total: {len(self.survivors)} Survivors, {len(self.obstacles)} Debris",
            fontsize=14,
            fontweight="bold",
        )
        plt.tight_layout()

        output_file = "floor_distribution.png"
        plt.savefig(output_file, dpi=300, bbox_inches="tight")
        plt.close(fig)

        print(f"📈 Floor distribution chart saved to: {output_file}")


def main():
    """الدالة الرئيسية"""
    print("🎨 Simple Survivor Map Visualizer")
    print("=" * 60)
    print("=" * 60)

    if len(sys.argv) > 1:
        map_file = sys.argv[1]
    else:
        map_file = "data/saved_map.txt"

    visualizer = MapVisualizer(map_file)

    if visualizer.parse_map_file():
        print("\n📋 Available Options:")
        print("1. Floor distribution chart")
        print("2. Create_3d_collapsed_building")
        print("3. All (Recommended)")

        choice = input("\nSelect option (1-3, default=3): ").strip()

        if choice == "":
            choice = "3"

        if HAS_MATPLOTLIB:
            if choice in ["1", "3"]:
                visualizer.create_floor_distribution_chart()

            if choice in ["2", "3"]:
                visualizer.create_3d_collapsed_building()
        else:
            print("\n⚠️ Graphical visualizations require matplotlib.")
            print("💡 Install it with: pip install matplotlib")

        print("\n✅ Visualization complete!")

        if not HAS_MATPLOTLIB:
            print("\n💡 For visualizations, install: pip install matplotlib numpy")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️ Operation cancelled by user")
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback

        traceback.print_exc()
