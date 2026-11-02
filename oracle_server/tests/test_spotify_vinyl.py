import io
import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

import numpy as np
from PIL import Image

from app.services.spotify_service import spotify_service


class TestSpotifyVinylGeneration(unittest.TestCase):
    def test_vinyl_generation_dimensions_and_bytes(self):
        # Червона тестова обкладинка
        img = Image.new("RGB", (300, 300), color=(220, 30, 40))
        buf = io.BytesIO()
        img.save(buf, format="JPEG")
        img_bytes = buf.getvalue()

        raw_bytes, ambi_rgb, ambi2_rgb, ambi3_rgb, is_mono = spotify_service._create_vinyl_art_bytes(img_bytes)

        # 1. Розмір буфера рівно 136 * 136 * 2 = 36992 байти
        expected_len = 136 * 136 * 2
        self.assertEqual(len(raw_bytes), expected_len)
        self.assertFalse(is_mono)

        # 2. Відсутність білого переповнення (жодного випадкового 0xFFFF у чистому чорному або середніх тонах)
        pixels = np.frombuffer(raw_bytes, dtype=">u2").reshape((136, 136))
        
        # Зовнішній радіус r > 67 px має бути суворо чорним (0x0000)
        self.assertEqual(int(pixels[0, 0]), 0)
        self.assertEqual(int(pixels[0, 135]), 0)
        self.assertEqual(int(pixels[135, 0]), 0)
        self.assertEqual(int(pixels[135, 135]), 0)

        # Центр r <= 4 px має бути суворо чорним (0x0000)
        center_y, center_x = 67, 67
        self.assertEqual(int(pixels[center_y, center_x]), 0)

        # Обкладинка не повинна переповнюватися в білий колір (0xFFFF)
        art_px = pixels[67, 40]
        self.assertNotEqual(int(art_px), 0xFFFF)
        # Червоний канал переважає у RGB565 (старші 5 біт)
        r_val = (int(art_px) >> 11) & 0x1F
        b_val = int(art_px) & 0x1F
        self.assertGreater(r_val, b_val)

        # 3. Кольори ambi_rgb відповідають червоній обкладинці (r > 200, b < 60)
        self.assertGreater(ambi_rgb[0], 200)
        self.assertLess(ambi_rgb[2], 60)

    def test_vinyl_generation_blue_cover(self):
        # Синя тестова обкладинка
        img = Image.new("RGB", (300, 300), color=(20, 60, 230))
        buf = io.BytesIO()
        img.save(buf, format="JPEG")
        img_bytes = buf.getvalue()

        raw_bytes, ambi_rgb, ambi2_rgb, ambi3_rgb, is_mono = spotify_service._create_vinyl_art_bytes(img_bytes)

        self.assertEqual(len(raw_bytes), 136 * 136 * 2)
        # Синій колір повинен домінувати
        self.assertGreater(ambi_rgb[2], 200)
        self.assertLess(ambi_rgb[0], 60)


if __name__ == "__main__":
    unittest.main()
