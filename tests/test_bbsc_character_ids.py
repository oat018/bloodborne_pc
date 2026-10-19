from paths import ROOT
import concurrent.futures
import contextlib
import http.client
import io
import json
import sqlite3
import subprocess
import sys
import tempfile
import threading
from types import SimpleNamespace
import unittest
from pathlib import Path

sys.path.insert(0, str(ROOT / 'scripts' / 'bbsc_gameserver'))
import bbsc_gameserver as server


class CharacterIdTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'characters.sqlite3'
        self.previous = getattr(server, 'CHARACTER_IDS', None)
        self.addCleanup(setattr, server, 'CHARACTER_IDS', self.previous)

    def request(self, count=1):
        return server.ROUTES['/basic_utils/sync_chara_id'](
            json.dumps({'MessageId': 'SyncCharaIdRequest', 'CharaIdNum': count}))

    def test_native_reply_contains_a_positive_exact_integer(self):
        self.start_store()
        reply = self.request()
        self.assertEqual(reply['ResKind'], 0)
        self.assertEqual(len(reply['PublishCharacterIdList']), 1)
        value = reply['PublishCharacterIdList'][0]['PublishCharaId']
        self.assertIs(type(value), int)
        self.assertGreater(value, 0)
        self.assertLessEqual(value, 9007199254740991)

    def start_store(self):
        server.CHARACTER_IDS = server.CharacterIdStore(str(self.path))

    def test_integral_json_float_count_matches_native_numeric_one(self):
        self.start_store()
        reply = self.request(1.0)
        self.assertEqual(reply['ResKind'], 0)
        self.assertEqual(len(reply['PublishCharacterIdList']), 1)

    def test_issued_ids_survive_restart_and_are_not_reused(self):
        self.start_store()
        first = self.request(10)['PublishCharacterIdList']
        second = json.loads(subprocess.check_output([
            sys.executable, '-c',
            'import sys,json; sys.path.insert(0,sys.argv[1]); '
            'import bbsc_gameserver as s; '
            's.CHARACTER_IDS=s.CharacterIdStore(sys.argv[2]); '
            'print(json.dumps(s.ROUTES["/basic_utils/sync_chara_id"]('
            '\'{"CharaIdNum":10}\')))',
            str(ROOT / 'scripts' / 'bbsc_gameserver'), str(self.path),
        ], text=True))['PublishCharacterIdList']
        values = [entry['PublishCharaId'] for entry in first + second]
        self.assertEqual(len(set(values)), 20)
        with contextlib.closing(sqlite3.connect(self.path)) as db:
            self.assertEqual({row[0] for row in db.execute('SELECT id FROM character_ids')},
                             set(values))

    def test_parallel_requests_do_not_lose_reservations(self):
        self.start_store()
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            replies = list(pool.map(lambda _: self.request(), range(20)))
        values = [reply['PublishCharacterIdList'][0]['PublishCharaId'] for reply in replies]
        self.assertEqual(len(set(values)), 20)
        with contextlib.closing(sqlite3.connect(self.path)) as db:
            self.assertEqual(db.execute('SELECT count(*) FROM character_ids').fetchone()[0], 20)

    def test_invalid_counts_do_not_issue_ids(self):
        self.start_store()
        for count in (None, False, True, 0, -1, 11, 1.5, float('inf'), float('nan'), '1', [], {}):
            with self.subTest(count=count):
                reply = self.request(count)
                self.assertNotEqual(reply['ResKind'], 0)
                self.assertEqual(reply['PublishCharacterIdList'], [])
        with contextlib.closing(sqlite3.connect(self.path)) as db:
            self.assertEqual(db.execute('SELECT count(*) FROM character_ids').fetchone()[0], 0)

    def test_unavailable_or_failed_store_never_returns_success(self):
        server.CHARACTER_IDS = None
        self.assertNotEqual(self.request()['ResKind'], 0)
        self.start_store()
        self.path.unlink()
        self.path.mkdir()
        reply = self.request()
        self.assertNotEqual(reply['ResKind'], 0)
        self.assertEqual(reply['PublishCharacterIdList'], [])

    def test_corrupt_database_is_not_replaced_with_empty_state(self):
        self.path.write_bytes(b'broken database')
        with self.assertRaises(sqlite3.DatabaseError):
            self.start_store()
        self.assertEqual(self.path.read_bytes(), b'broken database')

    def test_http_publishes_only_committed_ids_and_reports_storage_failure(self):
        self.start_store()
        previous = server.ARGS
        self.addCleanup(setattr, server, 'ARGS', previous)
        server.ARGS = SimpleNamespace(advertise=None, bbhost=False, verbose=False)
        httpd = server.ThreadingHTTPServer(('127.0.0.1', 0), server.Handler)
        worker = threading.Thread(target=httpd.serve_forever)
        worker.start()
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                for failed in (False, True):
                    if failed:
                        self.path.unlink()
                        self.path.mkdir()
                    client = http.client.HTTPConnection(*httpd.server_address, timeout=5)
                    try:
                        client.request('POST', '/basic_utils/sync_chara_id',
                                       json.dumps({'CharaIdNum': 1}),
                                       {'Content-Type': 'application/json'})
                        response = client.getresponse()
                        reply = json.loads(response.read())
                        self.assertEqual(response.status, 200)
                        if failed:
                            self.assertNotEqual(reply['ResKind'], 0)
                            self.assertEqual(reply['PublishCharacterIdList'], [])
                        else:
                            value = reply['PublishCharacterIdList'][0]['PublishCharaId']
                            with contextlib.closing(sqlite3.connect(self.path)) as db:
                                self.assertEqual(db.execute(
                                    'SELECT id FROM character_ids WHERE id=?',
                                    (value,)).fetchone(), (value,))
                    finally:
                        client.close()
        finally:
            httpd.shutdown()
            worker.join()
            httpd.server_close()


if __name__ == '__main__':
    unittest.main()
