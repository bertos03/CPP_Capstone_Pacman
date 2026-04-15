#!/usr/bin/env python3

import math
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "data" / "pacman_frames"
OUT_DIR.mkdir(parents=True, exist_ok=True)

CANVAS_SIZE = 512
OUTPUT_SIZE = 100

SOURCE_PATHS = {
    "down": Path(
        "/Users/robert/Downloads/ChatGPT Image 15. Apr. 2026, 20_51_16.png"
    ),
    "up": Path(
        "/Users/robert/Downloads/ChatGPT Image 15. Apr. 2026, 20_51_16 Kopie.png"
    ),
    "left": Path(
        "/Users/robert/Downloads/ChatGPT Image 15. Apr. 2026, 20_51_16 Kopie 2.png"
    ),
    "right": Path(
        "/Users/robert/Downloads/ChatGPT Image 15. Apr. 2026, 20_51_16 Kopie 3.png"
    ),
}


CONFIGS = {
    "down": {
        "parts": {
            "body": {"rect": (0.18, 0.00, 0.64, 0.78), "pivot": (0.50, 0.16)},
            "left_arm": {"rect": (0.00, 0.30, 0.24, 0.43), "pivot": (0.84, 0.10)},
            "right_arm": {"rect": (0.76, 0.30, 0.24, 0.43), "pivot": (0.16, 0.10)},
            "left_leg": {"rect": (0.20, 0.62, 0.22, 0.38), "pivot": (0.58, 0.10)},
            "right_leg": {"rect": (0.52, 0.58, 0.26, 0.42), "pivot": (0.42, 0.10)},
        },
        "order": ["body", "left_leg", "right_leg", "left_arm", "right_arm"],
        "frames": {
            0: {
                "body": {"translate": (0, -6), "rotate": 0.0},
                "left_arm": {"translate": (-8, -2), "rotate": 22.0},
                "right_arm": {"translate": (9, 7), "rotate": -24.0},
                "left_leg": {"translate": (-6, 9), "rotate": 16.0},
                "right_leg": {"translate": (8, -7), "rotate": -18.0},
            },
            1: {
                "body": {"translate": (0, 0), "rotate": 0.0},
                "left_arm": {"translate": (0, 0), "rotate": 0.0},
                "right_arm": {"translate": (0, 0), "rotate": 0.0},
                "left_leg": {"translate": (0, 0), "rotate": 0.0},
                "right_leg": {"translate": (0, 0), "rotate": 0.0},
            },
            2: {
                "body": {"translate": (0, -2), "rotate": 0.0},
                "left_arm": {"translate": (-5, 7), "rotate": -24.0},
                "right_arm": {"translate": (7, -3), "rotate": 21.0},
                "left_leg": {"translate": (-4, -6), "rotate": -18.0},
                "right_leg": {"translate": (6, 10), "rotate": 16.0},
            },
        },
    },
    "up": {
        "parts": {
            "body": {"rect": (0.16, 0.00, 0.68, 0.80), "pivot": (0.50, 0.16)},
            "left_arm": {"rect": (0.00, 0.30, 0.22, 0.42), "pivot": (0.82, 0.10)},
            "right_arm": {"rect": (0.78, 0.30, 0.22, 0.42), "pivot": (0.18, 0.10)},
            "left_leg": {"rect": (0.20, 0.64, 0.22, 0.36), "pivot": (0.56, 0.10)},
            "right_leg": {"rect": (0.54, 0.64, 0.22, 0.36), "pivot": (0.44, 0.10)},
        },
        "order": ["body", "left_leg", "right_leg", "left_arm", "right_arm"],
        "frames": {
            0: {
                "body": {"translate": (0, -6), "rotate": 0.0},
                "left_arm": {"translate": (-8, -2), "rotate": 20.0},
                "right_arm": {"translate": (8, 6), "rotate": -22.0},
                "left_leg": {"translate": (-5, 8), "rotate": 14.0},
                "right_leg": {"translate": (6, -6), "rotate": -16.0},
            },
            1: {
                "body": {"translate": (0, 0), "rotate": 0.0},
                "left_arm": {"translate": (0, 0), "rotate": 0.0},
                "right_arm": {"translate": (0, 0), "rotate": 0.0},
                "left_leg": {"translate": (0, 0), "rotate": 0.0},
                "right_leg": {"translate": (0, 0), "rotate": 0.0},
            },
            2: {
                "body": {"translate": (0, -2), "rotate": 0.0},
                "left_arm": {"translate": (-6, 6), "rotate": -22.0},
                "right_arm": {"translate": (6, -2), "rotate": 20.0},
                "left_leg": {"translate": (-4, -6), "rotate": -16.0},
                "right_leg": {"translate": (6, 8), "rotate": 14.0},
            },
        },
    },
    "left": {
        "parts": {
            "body": {"rect": (0.20, 0.00, 0.56, 0.84), "pivot": (0.50, 0.20)},
            "front_arm": {"rect": (0.00, 0.34, 0.32, 0.38), "pivot": (0.84, 0.10)},
            "back_arm": {"rect": (0.72, 0.34, 0.18, 0.34), "pivot": (0.22, 0.10)},
            "front_leg": {"rect": (0.24, 0.58, 0.56, 0.42), "pivot": (0.36, 0.10)},
            "back_leg": {"rect": (0.10, 0.72, 0.18, 0.24), "pivot": (0.54, 0.08)},
        },
        "order": ["back_arm", "body", "back_leg", "front_leg", "front_arm"],
        "frames": {
            0: {
                "body": {"translate": (0, -5), "rotate": 0.0},
                "front_arm": {"translate": (-6, 4), "rotate": 20.0},
                "back_arm": {"translate": (3, -5), "rotate": -18.0},
                "front_leg": {"translate": (-9, 8), "rotate": 18.0},
                "back_leg": {"translate": (6, -7), "rotate": -16.0},
            },
            1: {
                "body": {"translate": (0, 0), "rotate": 0.0},
                "front_arm": {"translate": (0, 0), "rotate": 0.0},
                "back_arm": {"translate": (0, 0), "rotate": 0.0},
                "front_leg": {"translate": (0, 0), "rotate": 0.0},
                "back_leg": {"translate": (0, 0), "rotate": 0.0},
            },
            2: {
                "body": {"translate": (0, -1), "rotate": 0.0},
                "front_arm": {"translate": (1, -5), "rotate": -18.0},
                "back_arm": {"translate": (1, 4), "rotate": 16.0},
                "front_leg": {"translate": (-2, -7), "rotate": -18.0},
                "back_leg": {"translate": (8, 7), "rotate": 15.0},
            },
        },
    },
    "right": {
        "parts": {
            "body": {"rect": (0.24, 0.00, 0.56, 0.84), "pivot": (0.50, 0.20)},
            "front_arm": {"rect": (0.68, 0.34, 0.32, 0.38), "pivot": (0.16, 0.10)},
            "back_arm": {"rect": (0.10, 0.34, 0.18, 0.34), "pivot": (0.78, 0.10)},
            "front_leg": {"rect": (0.20, 0.58, 0.56, 0.42), "pivot": (0.64, 0.10)},
            "back_leg": {"rect": (0.72, 0.72, 0.18, 0.24), "pivot": (0.46, 0.08)},
        },
        "order": ["back_arm", "body", "back_leg", "front_leg", "front_arm"],
        "frames": {
            0: {
                "body": {"translate": (0, -5), "rotate": 0.0},
                "front_arm": {"translate": (6, 4), "rotate": -20.0},
                "back_arm": {"translate": (-3, -5), "rotate": 18.0},
                "front_leg": {"translate": (9, 8), "rotate": -18.0},
                "back_leg": {"translate": (-6, -7), "rotate": 16.0},
            },
            1: {
                "body": {"translate": (0, 0), "rotate": 0.0},
                "front_arm": {"translate": (0, 0), "rotate": 0.0},
                "back_arm": {"translate": (0, 0), "rotate": 0.0},
                "front_leg": {"translate": (0, 0), "rotate": 0.0},
                "back_leg": {"translate": (0, 0), "rotate": 0.0},
            },
            2: {
                "body": {"translate": (0, -1), "rotate": 0.0},
                "front_arm": {"translate": (-1, -5), "rotate": 18.0},
                "back_arm": {"translate": (-1, 4), "rotate": -16.0},
                "front_leg": {"translate": (2, -7), "rotate": 18.0},
                "back_leg": {"translate": (-8, 7), "rotate": -15.0},
            },
        },
    },
}


def run_magick(args, input_bytes=None):
    return subprocess.run(
        ["magick", *args], input=input_bytes, check=True, capture_output=True
    )


def load_centered_rgba(path: Path) -> bytearray:
    result = run_magick(
        [
            str(path),
            "-background",
            "none",
            "-gravity",
            "south",
            "-extent",
            f"{CANVAS_SIZE}x{CANVAS_SIZE}",
            "rgba:-",
        ]
    )
    return bytearray(result.stdout)


def save_rgba(buf: bytearray, out_path: Path) -> None:
    run_magick(
        [
            "-size",
            f"{CANVAS_SIZE}x{CANVAS_SIZE}",
            "-depth",
            "8",
            "rgba:-",
            "-filter",
            "point",
            "-resize",
            f"{OUTPUT_SIZE}x{OUTPUT_SIZE}",
            f"PNG32:{out_path}",
        ],
        input_bytes=bytes(buf),
    )


def alpha_bbox(buf: bytearray):
    min_x = CANVAS_SIZE
    min_y = CANVAS_SIZE
    max_x = -1
    max_y = -1
    for y in range(CANVAS_SIZE):
        row_start = y * CANVAS_SIZE * 4
        for x in range(CANVAS_SIZE):
            if buf[row_start + x * 4 + 3] > 0:
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)
    return (min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)


def rect_from_normalized(bbox, rect_spec):
    bbox_x, bbox_y, bbox_w, bbox_h = bbox
    rect_x = bbox_x + int(round(rect_spec[0] * bbox_w))
    rect_y = bbox_y + int(round(rect_spec[1] * bbox_h))
    rect_w = max(1, int(round(rect_spec[2] * bbox_w)))
    rect_h = max(1, int(round(rect_spec[3] * bbox_h)))
    rect_w = min(rect_w, CANVAS_SIZE - rect_x)
    rect_h = min(rect_h, CANVAS_SIZE - rect_y)
    return (rect_x, rect_y, rect_w, rect_h)


def crop_rgba(buf: bytearray, rect):
    rect_x, rect_y, rect_w, rect_h = rect
    cropped = bytearray(rect_w * rect_h * 4)
    for y in range(rect_h):
        source_index = ((rect_y + y) * CANVAS_SIZE + rect_x) * 4
        target_index = y * rect_w * 4
        cropped[target_index : target_index + rect_w * 4] = buf[
            source_index : source_index + rect_w * 4
        ]
    return cropped


def composite_rgba(dest: bytearray, src: bytearray, src_w: int, src_h: int, dx: int, dy: int):
    for sy in range(src_h):
        ty = sy + dy
        if ty < 0 or ty >= CANVAS_SIZE:
            continue
        for sx in range(src_w):
            tx = sx + dx
            if tx < 0 or tx >= CANVAS_SIZE:
                continue
            sidx = (sy * src_w + sx) * 4
            sa = src[sidx + 3]
            if sa == 0:
                continue
            didx = (ty * CANVAS_SIZE + tx) * 4
            da = dest[didx + 3]
            if da == 0 or sa == 255:
                dest[didx : didx + 4] = src[sidx : sidx + 4]
                continue

            sa_f = sa / 255.0
            da_f = da / 255.0
            out_a = sa_f + da_f * (1.0 - sa_f)
            if out_a <= 0.0:
                continue

            for channel in range(3):
                sc = src[sidx + channel] / 255.0
                dc = dest[didx + channel] / 255.0
                out_c = (sc * sa_f + dc * da_f * (1.0 - sa_f)) / out_a
                dest[didx + channel] = max(0, min(255, int(round(out_c * 255.0))))
            dest[didx + 3] = max(0, min(255, int(round(out_a * 255.0))))


def transform_part(crop: bytearray, rect, pivot_norm, transform):
    rect_x, rect_y, rect_w, rect_h = rect
    pivot_x = pivot_norm[0] * (rect_w - 1)
    pivot_y = pivot_norm[1] * (rect_h - 1)
    translate_x, translate_y = transform["translate"]
    angle = math.radians(transform["rotate"])
    cos_angle = math.cos(angle)
    sin_angle = math.sin(angle)

    corners = []
    for source_x, source_y in (
        (0.0, 0.0),
        (rect_w - 1.0, 0.0),
        (0.0, rect_h - 1.0),
        (rect_w - 1.0, rect_h - 1.0),
    ):
        offset_x = source_x - pivot_x
        offset_y = source_y - pivot_y
        dest_x = rect_x + pivot_x + translate_x + offset_x * cos_angle - offset_y * sin_angle
        dest_y = rect_y + pivot_y + translate_y + offset_x * sin_angle + offset_y * cos_angle
        corners.append((dest_x, dest_y))

    min_x = max(0, int(math.floor(min(x for x, _ in corners))) - 2)
    max_x = min(CANVAS_SIZE - 1, int(math.ceil(max(x for x, _ in corners))) + 2)
    min_y = max(0, int(math.floor(min(y for _, y in corners))) - 2)
    max_y = min(CANVAS_SIZE - 1, int(math.ceil(max(y for _, y in corners))) + 2)

    transformed = bytearray(CANVAS_SIZE * CANVAS_SIZE * 4)
    for dest_y in range(min_y, max_y + 1):
        for dest_x in range(min_x, max_x + 1):
            local_x = dest_x - (rect_x + pivot_x + translate_x)
            local_y = dest_y - (rect_y + pivot_y + translate_y)
            source_offset_x = local_x * cos_angle + local_y * sin_angle
            source_offset_y = -local_x * sin_angle + local_y * cos_angle
            source_x = int(round(source_offset_x + pivot_x))
            source_y = int(round(source_offset_y + pivot_y))
            if source_x < 0 or source_x >= rect_w or source_y < 0 or source_y >= rect_h:
                continue

            sidx = (source_y * rect_w + source_x) * 4
            alpha = crop[sidx + 3]
            if alpha == 0:
                continue

            didx = (dest_y * CANVAS_SIZE + dest_x) * 4
            transformed[didx : didx + 4] = crop[sidx : sidx + 4]
    return transformed


def build_frame(base: bytearray, bbox, config, frame_index: int):
    frame = bytearray(CANVAS_SIZE * CANVAS_SIZE * 4)
    transforms = config["frames"][frame_index]
    for part_name in config["order"]:
        part_config = config["parts"][part_name]
        rect = rect_from_normalized(bbox, part_config["rect"])
        crop = crop_rgba(base, rect)
        transformed = transform_part(crop, rect, part_config["pivot"], transforms[part_name])
        composite_rgba(frame, transformed, CANVAS_SIZE, CANVAS_SIZE, 0, 0)
    return frame


def create_sheet(frame_paths):
    subprocess.run(
        [
            "magick",
            "montage",
            *[str(frame_path) for frame_path in frame_paths],
            "-background",
            "none",
            "-tile",
            "3x4",
            "-geometry",
            "120x120+4+4",
            str(OUT_DIR / "sheet.png"),
        ],
        check=True,
    )


def main():
    written_frames = []
    for direction, source_path in SOURCE_PATHS.items():
        base = load_centered_rgba(source_path)
        bbox = alpha_bbox(base)
        config = CONFIGS[direction]
        for frame_index in range(3):
            frame = build_frame(base, bbox, config, frame_index)
            output_path = OUT_DIR / f"{direction}_{frame_index}.png"
            save_rgba(frame, output_path)
            written_frames.append(output_path)

    create_sheet(written_frames)
    print(f"Generated {len(written_frames)} frames in {OUT_DIR}")


if __name__ == "__main__":
    main()
